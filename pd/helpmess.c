/* helpmess.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Help Message Strings from PipeDream */

#include "common/gflags.h"

/* header file (exports - stop whinge on release compilation) */
#include "strings.h"

#ifdef MAKE_MESSAGE_FILE

/* print strings to file */
#define string(tag, id, value)              printf("m%d:%s\n", tag, value )

/* now come the help message strings for real */

static void
helpmess_strings(void)
{
/*
 * Interactive Help system.
 * Tokens as described in Style Guide
 */

/* Help from the icon bar icon */

string(92, help_iconbar,
                                                "This is the PipeDream 4 icon.|M"
                                                "Drag a file onto this icon to load it.|M"
                                                "Click Select to create a new document.|M"
                                                "Shift-click Select to view the Choices directory.|M"
                                                "Click Menu to select a document or to set Choices.|M");

/* Help from a dialog window */

string(93, help_dialog_window,                  "This is a PipeDream 4 dialogue box.|M"
                                                "Click OK (or press Return) to complete.|M"
                                                "Click Cancel (or press Escape) to cancel.|M"
                                                "Use Tab (or Shift-Tab) to move forwards "
                                                "(or backwards) between writeable fields.");

/* Help from a PipeDream window */

#if 0 /* both users commented out */
string(94, help_main_window,                    "This is a PipeDream 4 document window.|M");
#endif

string(95, help_drag_file_to_insert,            "Drag a file into the window to insert the file at the current cell.");

string(96, help_click_select_to,                "Click Select to ");

string(97, help_click_adjust_to,                "Click Ctrl-Select to ");

string(98, help_position_the_caret_in,          "position the caret in ");

string(99, help_insert_a_reference_to,          "insert a reference to ");

string(100, help_cell,                          "cell ");

string(930, help_drag_select_to_mark_block,     "Drag Select to mark a block of cells.|M"
                                                "Click Adjust to change which cells are marked.");

string(101, help_colh_inexpression_line,        "You are editing a formula in the formula editing line so this button cannot perform its associated command.");

string(102, help_colh_inexpression_box,         "You are editing a formula in the formula window so this button cannot perform its associated command.");

string(103, help_top_left_corner,               "Click Select to mark the whole document.|M"
                                                "Click Adjust to clear a marked block.|M");

string(104, help_col_border,                    "Double-click Select to mark this column.|M"
                                                "Drag Select horizontally in the column border to mark a number of columns.|M"
                                                "Click Adjust or Shift-click Select to change which columns are marked.");

string(105, help_row_border,                    "Double-click Select to mark this row.|M"
                                                "Drag Select vertically in the row border to mark a number of rows.|M"
                                                "Click Adjust or Shift-click Select to change which rows are marked.");

#if 0 /* no longer used */
string(106, help_double_row_border,             "Double-click Select to mark this row.|M");
#endif

string(107, help_row_is_page_break,             "This is a page break.|M");

string(108, help_row_is_hard_page_break,        "This row is an expanded hard page break.|M");

string(109, help_colh_contents_line,            "Click Select to start editing the current cell contents in the formula editing line.");

string(110, help_colh_contents_line_inexpression,
                                                "Click Select to position the caret within the formula editing line.|M" \
                                                "Drag Select to set a selection.|M" \
                                                "Click Adjust to set a selection.");

string(414, help_colh_transfer_to_window,       "Click Select to popup a menu of formula related topics.|M" \
                                                "Click Adjust to transfer the formula editing line contents to a formula window.");

string(415, help_colh_edit_in_window,           "Click Select to popup a menu of formula related topics.|M" \
                                                "Click Adjust to edit the current cell contents in a formula window.");

string(416, help_drag_column_width,             "Drag Select to alter the width of column ");
string(417, help_drag_right_margin,             "Drag Select to alter the right margin of column ");
string(418, help_double_right_margin,           "Double-click Select to link the right margin to the columns width.|M");

/* Help from a PipeDream toolbar */

string(900, colh_button_replicd,                "Click Select to replicate the top row of a marked block down throughout the rest of the block.|M"
                                                "Click Adjust to replicate the bottom row of a marked block up throughout the rest of the block.");
string(901, colh_button_replicr,                "Click Select to replicate the left column of a marked block right throughout the rest of the block.|M"
                                                "Click Adjust to replicate the right column of a marked block left throughout the rest of the block.");
string(902, colh_button_graph,                  "Click Select to create a new chart.|M"
                                                "Click Adjust to set the chart options.");
string(903, colh_button_save,                   "Click Select to bring up the Save dialogue box.|M"
                                                "Click Adjust to save the file immediately.");
string(904, colh_button_print,                  "Click Select to bring up the Print dialogue box.|M"
                                                "Click Adjust to bring up the Page layout dialogue box.");
string(905, colh_button_ljustify,               "Click Select to apply left justification to a marked block (or current cell).|M"
                                                "Click Adjust to apply default justification to a marked block (or current cell).");
string(906, colh_button_cjustify,               "Click Select to apply centred justification to a marked block (or current cell).");
string(907, colh_button_rjustify,               "Click Select to apply right justification to a marked block (or current cell).");
string(908, colh_button_fjustify,               "Click Select to apply full justification (as from the Options dialogue box).");
string(909, colh_button_bold,                   "Click Select to insert bold at the caret or apply bold to a marked block.|M"
                                                "Click Adjust to remove bold from a marked block.");
string(910, colh_button_italic,                 "Click Select to insert italic at the caret or apply italic to a marked block.|M"
                                                "Click Adjust to remove italic from a marked block.");
string(911, colh_button_underlined,             "Click Select to insert underline at the caret or apply underline to a marked block.|M"
                                                "Click Adjust to remove underline from a marked block.");
string(912, colh_button_subscript,              "Click Select to insert subscript at the caret or apply subscript to a marked block.|M"
                                                "Click Adjust to remove subscript from a marked block.");
string(913, colh_button_superscript,            "Click Select to insert superscript at the caret or apply superscript to a marked block.|M"
                                                "Click Adjust to remove superscript from a marked block.");
string(914, colh_button_search,                 "Click Select to search and replace.|M"
                                                "Click Adjust to find the next occurence of the word being searched for.");
string(915, colh_button_sort,                   "Click Select to sort a marked block.|M"
                                                "Click Adjust to transpose a marked block.");
string(916, colh_button_spellcheck,             "Click Select to spell check this document.|M"
                                                "Click Adjust to browse through the spelling dictionary.");
string(917, colh_button_totext,                 "Click Select to force cells to text.|M"
                                                "Click Adjust to change between number and text.");
string(918, colh_button_tonumber,               "Click Select to force cells to number.|M"
                                                "Click Adjust to force cells to constant.");
string(431, colh_button_mark,                   "Click Select to mark the whole document.|M"
                                                "Click Adjust to clear a marked block.|M");
string(938, colh_button_copy,                   "Click Select to copy a marked block to the paste list.|M"
                                                "Click Adjust to move a marked block to the caret position.");
string(920, colh_button_delete,                 "Click Select to delete a marked block to the paste list.|M"
                                                "Click Adjust to clear the cells in a marked block.");
string(921, colh_button_paste,                  "Click Select to copy the top item on the paste list to the caret position.");

string(922, colh_button_edit_ok,                "Click Select to enter the formula you have typed on the formula editing line into the document.|M"
                                                "Click Adjust to transfer the formula into an editing window.");
string(923, colh_button_edit_cancel,            "Click Select to abandon editing the formula and leave the document unchanged.");

string(924, colh_button_font,                   "Click Select to set the font to be used in this document.|M"
                                                "Click Adjust to insert a local font change in a cell.");
string(925, colh_button_formatblock,            "Click Select to reformat a marked block (or current paragraph).");
string(926, colh_button_cmd_record,             "Click Select to start (or stop) recording a command file.");
string(927, colh_button_cmd_exec,               "Click Select to choose and execute a command file.");
string(928, colh_button_leadtrail,              "Click Select to apply leading characters to a marked block (or current cell).|M"
                                                "Click Adjust to apply trailing characters to a marked block (or current cell).");
string(929, colh_button_decplaces,              "Click Select to vary the number of decimal places displayed in a marked block (or current cell).|M"
                                                "Click Adjust to apply the default format to a marked block (or current cell).");

string(931, help_formwind_button_edit_ok,       "Click Select to enter the formula you have typed in the formula editing window into the document.");
string(932, help_formwind_button_edit_cancel,   "Click Select to abandon editing the formula and leave the document unchanged.");
string(933, help_formwind_button_function,      "Click Select to popup a menu of formula related topics.");
string(934, help_formwind_button_newline,       "Click Select to insert a line break in the formula.");
string(935, help_formwind_button_copy,          "Click Select to copy the selection to the clipboard.");
string(936, help_formwind_button_cut,           "Click Select to cut the selection to the clipboard.");
string(937, help_formwind_button_paste,         "Click Select to copy the contents of the clipboard to the caret.");
/* use 939 as next help */

/* use 561 as next if adding at end */

/* use 487 as next in middle */

} /* end of helpmess_strings() */

static void
fontselect_strings(void)
{
        /* 0         1         2         3         4         5         6         7         8 */
        /* 012345678901234567890123456789012345678901234567890123456789012345678901234567890 */
    fputs("FNTSELI13:"
          "You can adjust the height of the font using the arrows.|M"
          "Shift-click to change the width at the same time."
          "\n", stdout);
    fputs("FNTSELI14:"
          "You can adjust the height of the font using the arrows.|M"
          "Shift-click to change the width at the same time."
          "\n", stdout);
    fputs("FNTSELI15:"
          "You can enter the height in points that you want the font in."
          "\n", stdout);
    fputs("FNTSELI16:"
          "You can adjust the width of the font using the arrows.|M"
          "Shift-click to change the height at the same time."
          "\n", stdout);
    fputs("FNTSELI17:"
          "You can adjust the width of the font using the arrows.|M"
          "Shift-click to change the height at the same time."
          "\n", stdout);
    fputs("FNTSELI18:"
          "You can enter the width in points that you want the font in.|M"
          "\n", stdout);
    fputs("FNTSELI31:"
          "You can adjust the line spacing using the arrows.|M"
          "\n", stdout);
    fputs("FNTSELI32:"
          "You can adjust the line spacing using the arrows.|M"
          "\n", stdout);
    fputs("FNTSELI33:"
          "You can enter the value in points that you want the line spacing to be."
          "\n", stdout);
    fputs("FNTSELI34:"
          "This indicates if automatic line spacing (120% of the font height) is turned on.|M"
          "Click Select to select or deselect this option."
          "\n", stdout);
    fputs("FNTSELI35:"
          "Click Select to reflect the font, width and height choices in the sample text field."
          "\n", stdout);
    fputs("FNTSELI36:"
          "You can enter text here to see the effects of your font choice.|M"
          "\n", stdout);
}

#if !CROSS_COMPILE

extern int
main(void)
{
    fputs("# PipeDream 4 en_GB help messages (" __DATE__ ")" "\n", stdout);
    helpmess_strings();

    fputs("# PipeDream 4 en_GB tuned help strings for FontSelect" "\n", stdout);
    fontselect_strings();

    return(ferror(stdout)
               ? EXIT_FAILURE
               : EXIT_SUCCESS);
}

#endif /* CROSS_COMPILE */

#endif /* MAKE_MESSAGE_FILE */

/* end of helpmess.c */
