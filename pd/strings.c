/* strings.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Strings from PipeDream */

/* Stuart K. Swales 24-Feb-1989 */

#include "common/gflags.h"

/* header file (exports - stop whinge on release compilation) */
#include "strings.h"

#ifndef MAKE_MESSAGE_FILE

/* header file (internal) */
#define EXPORT_STRING_ALLOCATION
#undef __strings_h
#include "strings.h"

#include "msgs.h"

#include "errdef.h"

_Check_return_
_Ret_maybenull_
extern PC_U8Z
strings_lookup(
    S32 stringid)
{
    char buffer[256];
    PC_U8Z text = string_lookup(stringid);

    if(NULL == text)
    {
        consume_int(
            xsnprintf(buffer, elemof32(buffer),
                BAD_MESSAGE_FILE_STR
                         ? BAD_MESSAGE_FILE_STR
                         : "Missing or incomplete messages file at id %d",
                stringid));

        reperr(ERR_OUTPUTSTRING, buffer);

        text = "!!!";
    }

    return(text);
}

#endif /* MAKE_MESSAGE_FILE */

#ifdef MAKE_MESSAGE_FILE
/* print strings to file */
#define string(tag, id, value)              printf("m%d:%s\n", tag, value )
#else
/* string variable initialiser */
#define string(tag, id, value)              id = strings_lookup(tag)
#endif

#if RELEASED || defined(MAKE_MESSAGE_FILE)
#define duffstring(id) /* nuffink*/
#else
#define duffstring(id) P_U8 id = NULL;
#endif

/*
put duff ones here as duffstring(name_to_remove_sometime)
*/

/*duffstring(Paste_name_STR)*/ /* RIP 14.01.92 */

extern void __msgs_readfile(const char *filename);

extern void
strings_init(void)
{
    #ifndef MAKE_MESSAGE_FILE
    /* initialise message lookup system */
    msgs_init();
    #endif

/* now come the strings for real */

/* id 0 forbidden */

string(1, BAD_MESSAGE_FILE_STR,                 "Missing or incomplete messages file at id %d");

/* what set of countries shall we work with? */

string(2, DefaultLocale_STR,                    "ISO8859-1");
string(3, Unrecognised_arg_Zs_STR,              "Unrecognised arg -%s");

/* names of files used by PipeDream */

/* dictionaries go in this subdirectory */
string(8,  DICTS_SUBDIR_STR,                    "Dicts");
string(9,  MASTERDICT_STR,                      "Master");
string(10, USERDICT_STR,                        "UserDict");

/* dictionary definition files go in this subdirectory */
string(11, DICTDEFNS_SUBDIR_STR,                "DictDefn");
string(12, DEFAULT_DICTDEFN_FILE_STR,           "Latin1"); /* <<< can be Deutsch,English etc. */
/*string(13, spare_13_STR, "");*/

/* ini templates go in this subdirectory */
string(410, LTEMPLATE_SUBDIR_STR,               "Templates");
string(412, DEFAULT_LTEMPLATE_FILE_STR,         "Text");

/* default choices files - saved & loaded on path */
string(15, CHOICES_FILE_STR,                    "Choices");
string(16, PREFERRED_FILE_STR,                  "Preferred");

/* command files go in this subdirectory */
string(17, MACROS_SUBDIR_STR,                   "CmdFiles");

/* startup command file */
string(18, INITEXEC_STR,                        "Key");

/* printer drivers are stored in this subdirectory */
string(371, PDRIVERS_SUBDIR_STR,                "PDrivers");

/* external refs are resolved from this subdirectory */
string(406, EXTREFS_SUBDIR_STR,                 "Library");

string(22, DUMP_FILE_STR,                       "DumpedFile");
string(25, UNTITLED_ZD_STR,                     "Untitled%d");
string(26, ambiguous_tag_STR,                   "$$$AmbiguousTag");

string(41, PRESSANYKEY_STR,                     "Press any key");
string(42, Initialising_STR,                    "Initialising...");
string(113, Debug_STR,                          "Debug");

string(50, FZd_STR,                             "F%d");
string(51, New_width_STR,                       "New width");
string(52, New_right_margin_position_STR,       "New right margin position");
string(28, Zld_one_word_found_STR,              "%ld word found");
string(29, Zld_many_words_found_STR,            "%ld words found");
string(30, Zld_one_string_found_STR,            "%ld string found");
string(31, Zld_many_strings_found_STR,          "%ld strings found");
string(32, Zld_one_string_hidden_STR,           "%ld string hidden");
string(33, Zld_many_strings_hidden_STR,         "%ld strings hidden");
string(61, Zd_unrecognised_word_STR,            "%d unrecognised word");
string(62, Zd_unrecognised_words_STR,           "%d unrecognised words");
string(34, Zd_word_added_to_user_dict_STR,      "%d word added to user dictionary");
string(35, Zd_words_added_to_user_dict_STR,     "%d words added to user dictionary");
string(63, Zs_of_Zs_STR,                        "%s of %s");
string(64, Zs_complete_STR,                     "... %s complete ...");

string(65, Page_Zd_wait_between_pages_STR,      "Page %d.. Press M=Miss page, A=All pages, Return=Print this page, Escape=stop print");
string(66, Miss_Page_Chars_STR,                 "M"); /* e.g. German has "UU" for Uberspringen (properly umlauted) */
string(67, All_Pages_Chars_STR,                 "A");

string(53, Zd_file_edited_in_Zs_are_you_sure_STR,   "There is %d document edited but not saved in %s.");
string(54, Zd_files_edited_in_Zs_are_you_sure_STR,  "There are %d documents edited but not saved in %s.");
string(447,query_quit_SDC_Q_STR,                "Do you want to save the changes before exiting" " "\
                                                "or exit now, discarding all unsaved changes?"); /* yes, longer multi-line is OK here */

string(68, save_edited_file_Zs_SDC_S_STR,       "'%s' has been modified.");
string(446,save_edited_file_SDC_Q_STR,          "Do you want to save it?");

string(424,save_edited_chart_Zs_YN_S_STR,       "'%s' has not been saved.");
string(445,save_edited_chart_YN_Q_STR,          "Do you want to save it?");

string(423,close_dependent_charts_DC_S_STR,     "This document has dependent unsaved charts.");
string(442,close_dependent_charts_DC_Q_STR,     "Do you want to discard them?");

string(69, close_dependent_files_YN_S_STR,      "This document has dependent documents.");
string(444,close_dependent_files_YN_Q_STR,      "Do you want to close it?");

string(70, close_dependent_links_YN_S_STR,      "This document has dependent HotLinks.");
string(443,close_dependent_links_YN_Q_STR,      "Do you want to close it?");

string(71, load_supporting_Zs_YN_S_STR,         "File '%s' has a name that would conflict with another document.");
string(448,load_supporting_YN_Q_STR,            "Do you want to load it?");

string(72, name_supporting_Zs_YN_S_STR,         "This document would be given a name that would conflict with another document.");
string(449,name_supporting_YN_Q_STR,            "Do you want to rename it?");

/* -------------------------- dialog boxes ------------------------------- */

/* Titles to be poked into dialog boxes */

/* string(75, Create_user_dictionary_dialog_STR,   "Create user dictionary"); */
/* string(76, Open_user_dictionary_dialog_STR,     "Open user dictionary");   */
/* string(77, Close_user_dictionary_dialog_STR,    "Close user dictionary");  */
/* string(78, Lock_dictionary_dialog_STR,          "Lock dictionary");        */
/* string(79, Unlock_dictionary_dialog_STR,        "Unlock dictionary");      */
string(80, Dump_STR,                            "Dump");
string(81, Dumping_STR,                         "Dumping");
string(82, Merge_STR,                           "Merge");
string(83, Merging_STR,                         "Merging");
string(84, Anagrams_STR,                        "Anagrams");
string(85, Subgrams_STR,                        "Subgrams");
#if 1
/* SKS after PD 4.12 26mar92 */
string(86, Opened_user_dictionaries_STR,        "Opened dictionaries");
#else
string(86, Opened_user_dictionaries_STR,        "Opened user dictionaries");
#endif
string(87, Insert_word_in_user_dictionary_dialog_STR,   "Insert word in user dictionary");
string(88, Delete_word_from_user_dictionary_dialog_STR, "Delete word from user dictionary");
string(89, Insert_highlights_dialog_STR,                "Insert highlights");

/* Messages to be used for dproc__query */

string(90, Overwrite_existing_file_YN_Q_STR,    "Overwrite existing file?");

string(91, Cannot_store_block_YN_S_STR,         "Unable to store block to paste list.");
string(450,Cannot_store_block_YN_Q_STR,         "Continue deletion?");

/* Menu strings - keep in step with riscos.c menu offsets */

/* iconbar menu */
/* title is application name */
string(111, iconbar_menu_entries,               ">Info,Help...,>Documents,>New document,>Choices,Quit");

string(384, ic_menu_title,                      "Choices");

/* iconbar 'Windows' submenu */
string(112, iw_menu_title,                      "Documents");
string(114, iw_menu_prefix,                     "...");

/* Printer related */

string(121, No_RISC_OS_STR,                     "No RISC OS");
string(122, Zs_printer_driver_STR,              "%s %s%sprinter driver");

string(123, fp_only_validation_STR,             "a-+.0-9eE");
string(124, pointsize_STR,                      "%.3f");

string(126, memory_size_Zd_STR,                 "%dK");

string(130, _unable_to_create_new_document_STR, "- unable to create new document");

/* -------------------------------- Menus ---------------------------------- */

/* Top-level menu headings */

string(131, Blocks_top_level_menu_STR,          "Blocks");
string(439, Chart_top_level_menu_STR,           "Chart");
string(132, Cursor_top_level_menu_STR,          "Caret");
string(133, Edit_top_level_menu_STR,            "Edit");
string(134, Files_top_level_menu_STR,           "Files");
string(435, Help_top_level_menu_STR,            "Help");
string(135, Layout_top_level_menu_STR,          "Layout");
string(438, Print_top_level_menu_STR,           "Print");
string(136, Spell_top_level_menu_STR,           "Spell");

/* 'Blocks' submenu */

string(137, Mark_block_STR,                     "Mark block");
string(138, Clear_markers_STR,                  "Clear markers");
string(139, Copy_block_STR,                     "Copy block");
string(140, Copy_block_to_paste_list_STR,       "Copy block to paste list");
string(142, Move_block_STR,                     "Move block");
string(143, Delete_block_STR,                   "Delete block");
string(426, Clear_block_STR,                    "Clear block");
string(144, Replicate_down_STR,                 "Replicate down");
string(145, Replicate_right_STR,                "Replicate right");
string(429, Replicate_up_STR,                   "Replicate up");
string(430, Replicate_left_STR,                 "Replicate left");
string(146, Replicate_STR,                      "Replicate");
string(147, Sort_STR,                           "Sort");
string(148, Search_STR,                         "Search");
string(149, Next_match_STR,                     "Next match");
string(150, Previous_match_STR,                 "Previous match");
string(151, Set_protection_STR,                 "Set protection");
string(152, Clear_protection_STR,               "Clear protection");
string(153, Number_X_Text_STR,                  "Number <> Text");
string(175, ToText_STR,                         "Make text");
string(425, ToNumber_STR,                       "Make number");
string(428, ToConstant_STR,                     "Make constant");
string(154, Snapshot_STR,                       "Snapshot");
string(155, Word_count_STR,                     "Word count");

/* 'Caret' submenu */

string(156, Format_paragraph_STR,               "Format paragraph");
string(404, FormatBlock_STR,                    "Format block");
string(157, First_column_STR,                   "First column");
string(158, Last_column_STR,                    "Last column");
string(159, Next_word_STR,                      "Next word");
string(160, Previous_word_STR,                  "Previous word");
string(161, Centre_window_STR,                  "Centre window");
string(163, Save_position_STR,                  "Save position");
string(164, Restore_position_STR,               "Restore position");
string(165, Swap_position_and_caret_STR,        "Swap position and caret");
string(167, Go_to_slot_STR,                     "Go to cell");
string(168, Define_key_STR,                     "Define key");
string(169, Define_function_key_STR,            "Define function key");
string(170, Define_command_STR,                 "Define command");

/* RJM says these macro labels need changing when everyone fancies a mega-compile */
string(171, Do_macro_file_STR,                  "Do command file");
string(172, Record_macro_file_STR,              "Record command file");

/* 'Choices' submenu */

string(431, Classic_menus_STR,                  "Classic menus");
string(199, Short_menus_STR,                    "Short menus");
string(372, ChangeMenus_STR,                    "Change menus");
string(250, Auto_check_STR,                     "Auto check");
string(254, Display_user_dictionaries_STR,      "Dictionaries");
string(208, Auto_recalculation_STR,             "Auto recalc");
string(420, Auto_chart_recalculation_STR,       "Auto chart update");
string(434, New_chart_files_STR,                "New chart files");
string(201, Colours_STR,                        "Colours");
string(181, Overtype_STR,                       "Overtype");
string(141, Size_of_paste_list_STR,             "Paste depth");

/* 'Edit' submenu */

string(173, Delete_character_STR,               "Delete character");
string(174, Insert_space_STR,                   "Insert space");
string(176, Delete_word_STR,                    "Delete word");
string(177, Delete_to_end_of_slot_STR,          "Delete to end of cell");
string(178, Delete_row_STR,                     "Delete row");
string(179, Insert_row_STR,                     "Insert row");
string(180, Paste_STR,                          "Paste");
string(183, Swap_case_STR,                      "Swap case");
string(184, Edit_formula_STR,                   "Edit formula");
string(185, Insert_reference_STR,               "Insert reference");
string(186, Split_line_STR,                     "Split line");
string(187, Join_lines_STR,                     "Join lines");
string(188, Insert_row_in_column_STR,           "Insert row in column");
string(189, Delete_row_in_column_STR,           "Delete row in column");
string(190, Insert_column_STR,                  "Insert column");
string(191, Delete_column_STR,                  "Delete column");
string(192, Add_column_STR,                     "Add column");
string(193, Insert_page_break_STR,              "Insert page break");
string(433, Insert_page_number_STR,             "Insert page number");
string(451, Insert_date_STR,                    "Insert date");
string(452, Insert_time_STR,                    "Insert time");
string(453, Insert_colour_change_STR,           "Insert colour");

/* 'Files' submenu */

string(194, Load_file_STR,                      "Load");
string(432, Insert_file_STR,                    "Insert file");
string(195, Save_file_STR,                      "Save");
string(196, Save_template_STR,                  "Save template");
string(197, Name_STR,                           "Rename file");
string(198, New_window_STR,                     "New window");
string(200, Options_STR,                        "Options");

string(202, Next_file_STR,                      "Next file");
string(203, Previous_file_STR,                  "Previous file");
string(204, Top_file_STR,                       "Top file");
string(205, Bottom_file_STR,                    "Bottom file");

string(207, Recalculate_STR,                    "Recalculate");

string(210, About_STR,                          "About");
string(211, Exit_STR,                           "Quit");

/* 'Layout' submenu */

string(215, Set_column_width_STR,               "Set column width");
string(216, Set_right_margin_STR,               "Set right margin");
string(217, Fix_row_STR,                        "Fix row");
string(218, Fix_column_STR,                     "Fix column");
string(219, Move_margin_right_STR,              "Move margin right");
string(220, Move_margin_left_STR,               "Move margin left");
string(403, Auto_width_STR,                     "Auto width");
string(408, Link_columns_STR,                   "Link columns");
string(221, Left_align_STR,                     "Left align");
string(222, Centre_align_STR,                   "Centre align");
string(223, Right_align_STR,                    "Right align");
string(224, LCR_align_STR,                      "LCR align");
string(225, Free_align_STR,                     "Free align");
string(226, Decimal_places_STR,                 "Decimal places");
string(227, Sign_brackets_STR,                  "Sign brackets");
string(228, Sign_minus_STR,                     "Sign minus");
string(229, Leading_characters_STR,             "Leading characters");
string(230, Trailing_characters_STR,            "Trailing characters");
string(231, Default_format_STR,                 "Default format");

/* 'Print' submenu */

string(232, Print_STR,                          "Print");
string(233, Set_parameter_STR,                  "Set parameter");
string(234, Page_layout_STR,                    "Page layout");
string(235, Printer_configuration_STR,          "Printer config");
string(236, Microspace_pitch_STR,               "Microspace pitch");
string(411, Edit_printer_driver_STR,            "Edit printer driver");
string(237, Printer_font_STR,                   "Font");
string(238, Insert_font_STR,                    "Local font");
string(239, Printer_line_spacing_STR,           "Line height");
string(240, Underline_STR,                      "Underline");
string(241, Bold_STR,                           "Bold");
string(242, Extended_sequence_STR,              "Extended sequence");
string(243, Italic_STR,                         "Italic");
string(244, Subscript_STR,                      "Subscript");
string(245, Superscript_STR,                    "Superscript");
string(246, Alternate_font_STR,                 "Alternate font");
string(247, User_defined_STR,                   "User defined");
string(248, Remove_highlights_STR,              "Remove highlights");
string(249, Highlight_block_STR,                "Highlight block");

/* 'Spell' submenu */

string(251, Check_document_STR,                 "Check document");
string(252, Delete_word_from_user_dict_STR,     "Delete word from user dictionary");
string(253, Insert_word_in_user_dict_STR,       "Insert word in user dictionary");
string(255, Browse_STR,                         "Browse");
string(256, Dump_dictionary_STR,                "Dump dictionary");
string(257, Merge_file_with_user_dict_STR,      "Merge file with user dictionary");
string(258, Create_user_dictionary_STR,         "Create user dictionary");
string(259, Open_user_dictionary_STR,           "Open user dictionary");
string(260, Close_user_dictionary_STR,          "Close user dictionary");
string(400, Flush_user_dictionary_STR,          "Flush dictionaries");
string(261, Pack_user_dictionary_STR,           "Pack user dictionary");
string(401, Dictionary_options_STR,             "User dictionary options");
string(262, Lock_dictionary_STR,                "Lock dictionary");
string(263, Unlock_dictionary_STR,              "Unlock dictionary");

/* 'Random' submenu */

string(264, Cursor_up_STR,                      "Caret up");
string(265, Cursor_down_STR,                    "Caret down");
string(266, Cursor_left_STR,                    "Caret left");
string(267, Cursor_right_STR,                   "Caret right");
string(268, Top_of_column_STR,                  "Top of column");
string(269, Bottom_of_column_STR,               "Bottom of column");
string(270, Start_of_slot_STR,                  "Start of cell");
string(271, End_of_slot_STR,                    "End of cell");
string(272, Scroll_up_STR,                      "Scroll up");
string(273, Scroll_down_STR,                    "Scroll down");
string(274, Scroll_left_STR,                    "Scroll left");
string(275, Scroll_right_STR,                   "Scroll right");
string(276, Page_up_STR,                        "Page up");
string(277, Page_down_STR,                      "Page down");
string(278, Enter_STR,                          "Enter");
string(279, Rubout_STR,                         "Rubout");
string(280, Next_column_STR,                    "Next column");
string(281, Previous_column_STR,                "Previous column");
string(282, Escape_STR,                         "Escape");
string(283, Pause_STR,                          "Pause");
string(284, Replace_STR,                        "Replace");
string(285, Next_window_STR,                    "Next window");
string(286, Close_window_STR,                   "Close window");

/* 'Help' submenu */

string(209, Help_STR,                           "Help");
string(441, Licence_STR,                        "Licence");
string(440, Interactive_help_STR,               "Interactive help");


string(287, Tidy_up_STR,                        "Reclaim memory");
string(405, Load_Template_STR,                  "Load template");

string(288, Mark_sheet_STR,                     "Mark sheet");

/* ------------------------------ Dialog boxes ------------------------------*/

/* thousands separator options */

string(289, thousands_none_STR,                 "none");
string(290, thousands_comma_STR,                ",");
string(291, thousands_dot_STR,                  ".");
string(292, thousands_space_STR,                "space");

/* printer type options */

string(293, printertype_RISC_OS_STR,            "RISC OS");
string(294, printertype_Parallel_STR,           "Parallel");
string(295, printertype_Serial_STR,             "Serial");
string(296, printertype_Network_STR,            "Network");
string(297, printertype_User_STR,               "User");

/* function key names */

string(298, F1_STR,                             "F1");
string(299, F2_STR,                             "F2");
string(300, F3_STR,                             "F3");
string(301, F4_STR,                             "F4");
string(302, F5_STR,                             "F5");
string(303, F6_STR,                             "F6");
string(304, F7_STR,                             "F7");
string(305, F8_STR,                             "F8");
string(306, F9_STR,                             "F9");
string(307, F10_STR,                            "F10");
string(308, F11_STR,                            "F11");
string(309, F12_STR,                            "F12");

string(310, Shift_F1_STR,                       "Shift F1");
string(311, Shift_F2_STR,                       "Shift F2");
string(312, Shift_F3_STR,                       "Shift F3");
string(313, Shift_F4_STR,                       "Shift F4");
string(314, Shift_F5_STR,                       "Shift F5");
string(315, Shift_F6_STR,                       "Shift F6");
string(316, Shift_F7_STR,                       "Shift F7");
string(317, Shift_F8_STR,                       "Shift F8");
string(318, Shift_F9_STR,                       "Shift F9");
string(319, Shift_F10_STR,                      "Shift F10");
string(320, Shift_F11_STR,                      "Shift F11");
string(321, Shift_F12_STR,                      "Shift F12");

string(322, Ctrl_F1_STR,                        "Ctrl F1");
string(323, Ctrl_F2_STR,                        "Ctrl F2");
string(324, Ctrl_F3_STR,                        "Ctrl F3");
string(325, Ctrl_F4_STR,                        "Ctrl F4");
string(326, Ctrl_F5_STR,                        "Ctrl F5");
string(327, Ctrl_F6_STR,                        "Ctrl F6");
string(328, Ctrl_F7_STR,                        "Ctrl F7");
string(329, Ctrl_F8_STR,                        "Ctrl F8");
string(330, Ctrl_F9_STR,                        "Ctrl F9");
string(331, Ctrl_F10_STR,                       "Ctrl F10");
string(332, Ctrl_F11_STR,                       "Ctrl F11");
string(333, Ctrl_F12_STR,                       "Ctrl F12");

string(334, Ctrl_Shift_F1_STR,                  "Ctrl-Shift F1");
string(335, Ctrl_Shift_F2_STR,                  "Ctrl-Shift F2");
string(336, Ctrl_Shift_F3_STR,                  "Ctrl-Shift F3");
string(337, Ctrl_Shift_F4_STR,                  "Ctrl-Shift F4");
string(338, Ctrl_Shift_F5_STR,                  "Ctrl-Shift F5");
string(339, Ctrl_Shift_F6_STR,                  "Ctrl-Shift F6");
string(340, Ctrl_Shift_F7_STR,                  "Ctrl-Shift F7");
string(341, Ctrl_Shift_F8_STR,                  "Ctrl-Shift F8");
string(342, Ctrl_Shift_F9_STR,                  "Ctrl-Shift F9");
string(343, Ctrl_Shift_F10_STR,                 "Ctrl-Shift F10");
string(344, Ctrl_Shift_F11_STR,                 "Ctrl-Shift F11");
string(345, Ctrl_Shift_F12_STR,                 "Ctrl-Shift F12");

/* Month names */

string(346, month_name_01_STR,                  "January");
string(347, month_name_02_STR,                  "February");
string(348, month_name_03_STR,                  "March");
string(349, month_name_04_STR,                  "April");
string(350, month_name_05_STR,                  "May");
string(351, month_name_06_STR,                  "June");
string(352, month_name_07_STR,                  "July");
string(353, month_name_08_STR,                  "August");
string(354, month_name_09_STR,                  "September");
string(355, month_name_10_STR,                  "October");
string(356, month_name_11_STR,                  "November");
string(357, month_name_12_STR,                  "December");

/* Day names */

string(476, day_name_01_STR,                    "Sunday");
string(477, day_name_02_STR,                    "Monday");
string(478, day_name_03_STR,                    "Tuesday");
string(479, day_name_04_STR,                    "Wednesday");
string(480, day_name_05_STR,                    "Thursday");
string(481, day_name_06_STR,                    "Friday");
string(482, day_name_07_STR,                    "Saturday");

string(483, day_endings_STR,                    "st nd rd th th th th th th th th th th th th th th th th th st nd rd th th th th th th th st");

/* page number formats */

string(435, PN_FMT1_STR,                        "#");
string(436, PN_FMT2_STR,                        "r#");
string(437, PN_FMT3_STR,                        "R#");

/* date formats */

string(454, DATE_FMT01_STR,                     "yyyy-mm-dd");
string(455, DATE_FMT02_STR,                     "dd.mm.yy");
string(456, DATE_FMT03_STR,                     "dd/mm/yy");
string(457, DATE_FMT04_STR,                     "dd Mmm yy");
string(458, DATE_FMT05_STR,                     "Mmm dd, yy");
string(459, DATE_FMT06_STR,                     "d Mmm yy");
string(460, DATE_FMT07_STR,                     "d Mmmm yy");
string(461, DATE_FMT08_STR,                     "Mmmm d, yy");
string(462, DATE_FMT09_STR,                     "d Mmmm yyyy");
string(463, DATE_FMT10_STR,                     "ddd Mmmm");
string(464, DATE_FMT11_STR,                     "Mmmm ddd");
string(465, DATE_FMT12_STR,                     "ddd Mmmm yy");
string(466, DATE_FMT13_STR,                     "Mmmm ddd, yy");
string(467, DATE_FMT14_STR,                     "ddd Mmmm yyyy");
string(468, DATE_FMT15_STR,                     "Mmmm ddd, yyyy");
string(469, DATE_FMT16_STR,                     "Mmm yy");
string(470, DATE_FMT17_STR,                     "Mmmm yy");
string(471, DATE_FMT18_STR,                     "Mmmm yyyy");
string(472, DATE_FMT19_STR,                     "dd.mm.yy hh:mm:ss");

/* time formats */

string(473, TIME_FMT1_STR,                      "hh:mm");
string(474, TIME_FMT2_STR,                      "hh:mm:ss");
string(475, TIME_FMT3_STR,                      "h:mm am;h:mm pm");

string(358, YES_STR,                            "Yes");
string(359, NO_STR,                             "No");

string(360, DEFAULTLEADING_STR,                 "\xA3"); /* pound char */
string(361, DEFAULTTRAILING_STR,                "%");
string(427, DEFAULTTEXTAT_STR,                  "@");

string(362, DEFAULTPSERVER_STR,                 "0.235");
string(363, DEFAULTITERLIMIT_STR,               "0.001");
string(364, DEFAULTITERCOUNT_STR,               "100");

string(365, lf_str,   "LF");
string(366, cr_str,   "CR");
string(367, crlf_str, "CR,LF");
string(368, lfcr_str, "LF,CR");

string(369, Edit_formula_in_window_STR,         "Edit formula in window");

string(370, SaveChoices_STR,                    "Save choices");

string(373, New_slot_contents_STR,              "New cell contents");
string(374, FullScreen_STR,                     "Full screen");
string(375, SpreadArray_STR,                    "Transpose block");
string(376, RepeatCommand_STR,                  "Repeat Command");

string(377, New_chart_STR,                      "New chart");
string(419, Chart_options_STR,                  "Chart options");
string(378, Add_to_chart_STR,                   "Add to chart");
string(379, Remove_from_chart_STR,              "Remove from chart");
string(380, Edit_chart_STR,                     "Edit chart");
string(381, Delete_chart_STR,                   "Delete chart");
string(382, Select_chart_STR,                   "Select chart");
string(383, Chart_STR,                          "Chart");

string(385, Page_left_STR,                      "Page left");
string(386, Page_right_STR,                     "Page right");

string(407, CustFuncErrStr,                     "Error in custom function, row %d: ");
string(421, PropagatedErrStr,                   "Propagated from %s: ");

string(500, function_menu_title,                "Functions");
string(501, function_menu_entries,              "123456789012|"
                                                "Database,"         "Date & time,"
                                                "Financial,"        "Lookup & reference,"   "String|"
                                                "Complex number,"   "Mathematical,"
                                                "Matrix,"           "Statistical,"          "Trigonometrical|"
                                                "Miscellaneous|"
                                                "Custom control,"   "Custom,"               "Names|"
                                                ">Define name,"     "Edit name"             );

string(502, menu_function_database_title,       "Database");
string(503, menu_function_date_title,           "Date & time");
string(504, menu_function_financial_title,      "Financial");
string(505, menu_function_lookup_title,         "Lookup");
string(506, menu_function_string_title,         "String");
string(507, menu_function_complex_title,        "Complex");
string(508, menu_function_maths_title,          "Maths.");
string(509, menu_function_matrix_title,         "Matrix");
string(510, menu_function_stat_title,           "Statistical");
string(511, menu_function_trig_title,           "Trigonom.");
string(512, menu_function_misc_title,           "Misc.");
string(513, menu_function_control_title,        "Control");
string(514, menu_function_custom_title,         "Custom");
string(515, menu_function_names_title,          "Names");
string(518, Edit_name_STR,                      "Edit name");
string(520, formwind_title_STR,                 "Formula window: %s");

string(521, menu_function_numinfo_title,        "Number cell");
string(522, menu_function_numinfo_entries,      "Cell value,"
                                                "Supporting cells,Supporting ranges, Supporting names,"
                                                "Dependent cells,Dependent ranges,Dependent names");
string(523, menu_function_numinfo_value_title,  "Cell value");
string(524, menu_function_numinfo_slr_title,    "Cells");
string(525, menu_function_numinfo_range_title,  "Ranges");
string(526, menu_function_numinfo_name_title,   "Names");

string(527, menu_function_textinfo_title,       "Text cell");
string(528, menu_function_textinfo_entries,     "Compile/Show error,"
                                                "Dependent cells,Dependent ranges,Dependent names");

string(560, menu_function_textinfo_title_blank, "Empty cell");

string(529, formwind_menu_title,                "Formula");

/* more function key-like strings */

string(530,   F_Print_STR,                      "Print");
string(531,  SF_Print_STR,                      "Shift Print");
string(532,  CF_Print_STR,                      "Ctrl Print");
string(533, CSF_Print_STR,                      "Ctrl-Shift Print");

string(534,   F_Insert_STR,                     "Insert");
string(535,  SF_Insert_STR,                     "Shift Insert");
string(536,  CF_Insert_STR,                     "Ctrl Insert");
string(537, CSF_Insert_STR,                     "Ctrl-Shift Insert");

string(538,   F_Home_STR,                       "Home");
string(539,  SF_Home_STR,                       "Shift Home");
string(540,  CF_Home_STR,                       "Ctrl Home");
string(541, CSF_Home_STR,                       "Ctrl-Shift Home");

string(542,   F_Copy_STR,                       "Copy");
string(543,  SF_Copy_STR,                       "Shift Copy");
string(544,  CF_Copy_STR,                       "Ctrl Copy");
string(545, CSF_Copy_STR,                       "Ctrl-Shift Copy");

string(546,   F_Delete_STR,                     "Delete");
string(547,  SF_Delete_STR,                     "Shift Delete");
string(548,  CF_Delete_STR,                     "Ctrl Delete");
string(549, CSF_Delete_STR,                     "Ctrl-Shift Delete");

string(550,   F_Backspace_STR,                  "Backspace");
string(551,  SF_Backspace_STR,                  "Shift Backspace");
/*string(552,  CF_Backspace_STR,                  "Ctrl Backspace"); sadly undistinguishable from ^H */
/*string(553, CSF_Backspace_STR,                  "Ctrl-Shift Backspace"); ditto */

string(554,   F_Tab_STR,                        "Tab");
string(555,  SF_Tab_STR,                        "Shift Tab");
string(556,  CF_Tab_STR,                        "Ctrl Tab");
string(557, CSF_Tab_STR,                        "Ctrl-Shift Tab");

string(558, ToNumber_STR,                       "Make number");
string(559, ToText_STR,                         "Make text");

/*
 * Interactive Help system.
 * Tokens as described in Style Guide
 */

/* Help from the icon bar icon */

string(92, help_iconbar,
                                                "\\TPipeDream icon.|M"
                                                "Drag a file onto this icon to load it.|M"
                                                "\\Screate a new document.|M"
                                                "Shift-click \\s to view the Choices directory.|M"
                                                "Click MENU to select a document or set Choices.|M");

/* Help from a dialog window */

string(93, help_dialog_window,                  "\\Wa PipeDream dialogue box.|M"
                                                "Click OK or press Return to complete.|M"
                                                "Click Cancel or press Escape to cancel.|M"
                                                "Use Tab (or Shift-Tab) to move forwards "
                                                "(or backwards) between writeable fields.");

/* Help from a PipeDream window */

string(94, help_main_window,                    "This is a PipeDream document \\w.|M");

string(95, help_drag_file_to_insert,            "Drag a file into the \\w to insert the file at the current cell.");

string(96, help_click_select_to,                "\\S");

string(97, help_click_adjust_to,                "Click Ctrl-\\s to ");

string(98, help_position_the_caret_in,          "position the caret in ");

string(99, help_insert_a_reference_to,          "insert a reference to ");

string(100, help_cell,                          "cell ");

string(930, help_drag_select_to_mark_block,     "\\Dmark a block of cells.|M"
                                                "\\Ato change which cells are marked.");

string(101, help_colh_inexpression_line,        "You are editing a formula in the formula editing line so this button cannot perform its associated command.");

string(102, help_colh_inexpression_box,         "You are editing a formula in the formula \\w so this button cannot perform its associated command.");

string(103, help_top_left_corner,               "\\Smark the whole document.|M"
                                                "\\Aclear a marked block.|M");

string(104, help_col_border,                    "Drag \\s along the column border to mark all rows in those columns.|M"
                                                "Double-click \\s to mark this column.|M");

string(105, help_drag_row_border,               "Drag \\s along the row border to mark all columns in those rows.|M");

string(106, help_double_row_border,             "Double-click \\s to mark this row.|M");

string(107, help_row_is_page_break,             "This is a page break.|M");

string(108, help_row_is_hard_page_break,        "This row is an expanded hard page break.|M");

string(109, help_colh_contents_line,            "\\Sstart editing the current cell contents in the formula editing line.");

string(110, help_colh_contents_line_inexpression,
                                                "\\Sposition the caret within the formula editing line.");

string(414, help_colh_transfer_to_window,       "\\Spopup a menu of formula related topics.|M" \
                                                "\\Atransfer the formula editing line contents to a formula \\w.");

string(415, help_colh_edit_in_window,           "\\Spopup a menu of formula related topics.|M" \
                                                "\\Aedit the current cell contents in a formula \\w.|M");

string(416, help_drag_column_width,             "\\Dalter the width of column ");
string(417, help_drag_right_margin,             "\\Dalter the right margin of column ");
string(418, help_double_right_margin,           "Double-click \\s to link the right margin to the columns width.|M");

/* Help from a PipeDream toolbar */

string(900, colh_button_replicd,                "\\Sreplicate the top row of a marked block down throughout the rest of the block.|M"
                                                "\\Areplicate the bottom row of a marked block up throughout the rest of the block.");
string(901, colh_button_replicr,                "\\Sreplicate the left column of a marked block right throughout the rest of the block.|M"
                                                "\\Areplicate the right column of a marked block left throughout the rest of the block.");
string(902, colh_button_graph,                  "\\Screate a new chart.|M"
                                                "\\Aset the chart options.");
string(903, colh_button_save,                   "\\Sbring up the Save dialogue box.|M"
                                                "\\Asave the file immediately.");
string(904, colh_button_print,                  "\\Sbring up the Print dialogue box.|M"
                                                "\\Abring up the Page layout dialogue box.");
string(905, colh_button_ljustify,               "\\Sapply left justification to a marked block (or current cell)."
                                                "\\Aapply default justification to a marked block (or current cell).");
string(906, colh_button_cjustify,               "\\Sapply centred justification to a marked block (or current cell).");
string(907, colh_button_rjustify,               "\\Sapply right justification to a marked block (or current cell).");
string(908, colh_button_fjustify,               "\\Sapply full justification (as from the Options dialogue box).");
string(909, colh_button_bold,                   "\\Sinsert bold at the caret or apply bold to a marked block.|M"
                                                "\\Aremove bold from a marked block.");
string(910, colh_button_italic,                 "\\Sinsert italic at the caret or apply italic to a marked block.|M"
                                                "\\Aremove italic from a marked block.");
string(911, colh_button_underlined,             "\\Sinsert underline at the caret or apply underline to a marked block.|M"
                                                "\\Aremove underline from a marked block.");
string(912, colh_button_subscript,              "\\Sinsert subscript at the caret or apply subscript to a marked block.|M"
                                                "\\Aremove subscript from a marked block.");
string(913, colh_button_superscript,            "\\Sinsert superscript at the caret or apply superscript to a marked block.|M"
                                                "\\Aremove superscript from a marked block.");
string(914, colh_button_search,                 "\\Ssearch and replace.|M"
                                                "\\Afind the next occurence of the word being searched for.");
string(915, colh_button_sort,                   "\\Ssort a marked block.|M"
                                                "\\Atranspose a marked block.");
string(916, colh_button_spellcheck,             "\\Sspell check this document.|M"
                                                "\\Abrowse through the spelling dictionary.");
string(917, colh_button_totext,                 "\\Sforce cells to text.|M"
                                                "\\Achange between number and text.");
string(918, colh_button_tonumber,               "\\Sforce cells to number.|M"
                                                "\\Aforce cells to constant.");
string(431, colh_button_mark,                   "\\Smark the whole document.|M"
                                                "\\Aclear a marked block.|M");
string(938, colh_button_copy,                   "\\Scopy a marked block to the paste list.|M"
                                                "\\Amove a marked block to the caret position.");
string(920, colh_button_delete,                 "\\Sdelete a marked block to the paste list.|M"
                                                "\\Aclear the cells in a marked block.");
string(921, colh_button_paste,                  "\\Scopy the top item on the paste list to the caret position.");

string(922, colh_button_edit_ok,                "\\Senter the formula you have typed on the formula editing line into the document.|M"
                                                "\\Atransfer the formula into an editing window.");
string(923, colh_button_edit_cancel,            "\\Sabandon editing the formula and leave the document unchanged.");

string(924, colh_button_font,                   "\\Sset the font to be used in this document.|M"
                                                "\\Ainsert a local font change in a cell.");
string(925, colh_button_formatblock,            "\\Sreformat a marked block (or current paragraph).");
string(926, colh_button_cmd_record,             "\\Sstart (or stop) recording a command file.");
string(927, colh_button_cmd_exec,               "\\Schoose and execute a command file.");
string(928, colh_button_leadtrail,              "\\Sapply leading characters to a marked block (or current cell).|M"
                                                "\\Aapply trailing characters to a marked block (or current cell).");
string(929, colh_button_decplaces,              "\\Svary the number of decimal places displayed in a marked block (or current cell)."
                                                "\\Aapply the default format to a marked block (or current cell).");

string(931, help_formwind_button_edit_ok,       "\\Senter the formula you have typed in the formula editing window into the document.");
string(932, help_formwind_button_edit_cancel,   "\\Sabandon editing the formula and leave the document unchanged.");
string(933, help_formwind_button_function,      "\\Spopup a menu of formula related topics.");
string(934, help_formwind_button_newline,       "\\Sinsert a line break in the formula.");
string(935, help_formwind_button_copy,          "\\Scopy the selection to the clipboard.");
string(936, help_formwind_button_cut,           "\\Scut the selection to the clipboard.");
string(937, help_formwind_button_paste,         "\\Scopy the contents of the clipboard to the caret.");
/* use 939 as next help */

/* use 561 as next if adding at end */

/* use 484 as next in middle */

} /* end of strings_init() */

#ifdef MAKE_MESSAGE_FILE

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
          "\\Sselect or deselect this option."
          "\n", stdout);
    fputs("FNTSELI35:"
          "\\Sreflect the font, width and height choices in the sample text field."
          "\n", stdout);
    fputs("FNTSELI36:"
          "You can enter text here to see the effects of your font choice.|M"
          "\n", stdout);
}

extern int
main(void)
{
    fputs("# PipeDream en_GB messages (" __DATE__ ")" "\n", stdout);
    strings_init();

    fputs("# PipeDream en_GB tuned help strings for FontSelect" "\n", stdout);
    fontselect_strings();

    return(ferror(stdout)
               ? EXIT_FAILURE
               : EXIT_SUCCESS);
}

#endif /* MESSAGE_FILE */

/* end of strings.c */
