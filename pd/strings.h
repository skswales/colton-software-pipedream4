/* strings.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __strings_h
#define __strings_h

extern void
strings_init(void);

#ifndef MAKE_MESSAGE_FILE

extern void
reperr_init(void);

_Check_return_
_Ret_maybenull_
extern PC_U8Z
strings_lookup(
    S32 stringid);

#endif

#include "cmodules/stringlk.h"

#ifndef __pd_strings_h /* stop multiple def warnings */
#define __pd_strings_h

/* Macro definitions */

/* Both NorCroft and Microsoft give better code for shorter inline strings
 * as opposed to externals -- only language independent ones here!
*/

#define NULLSTR         ""

#define UNULLSTR        ((uchar *) NULLSTR)
#define CR_STR          "\x0D"
#define SPACE_STR       " "

#define WILD_STR        "*"

#endif /* __pd_strings_h */

#ifdef MAKE_MESSAGE_FILE
#define stringdef(id)    /* nothing */
#else
#if defined(EXPORT_STRING_ALLOCATION)
#undef  stringdef
#define stringdef(id)               PC_U8 id = NULL;
#else
#define stringdef(id)    extern     PC_U8 id;
#endif
#endif

stringdef(BAD_MESSAGE_FILE_STR)

stringdef(DefaultLocale_STR)
stringdef(Unrecognised_arg_Zs_STR)

/* names of files used by PipeDream */

stringdef(DICTS_SUBDIR_STR)
stringdef(MASTERDICT_STR)
stringdef(USERDICT_STR)

stringdef(DICTDEFNS_SUBDIR_STR)
stringdef(DEFAULT_DICTDEFN_FILE_STR)

stringdef(LTEMPLATE_SUBDIR_STR)
stringdef(DEFAULT_LTEMPLATE_FILE_STR)

stringdef(CHOICES_FILE_STR)
stringdef(PREFERRED_FILE_STR)

stringdef(MACROS_SUBDIR_STR)
stringdef(INITEXEC_STR)

stringdef(PDRIVERS_SUBDIR_STR)

stringdef(EXTREFS_SUBDIR_STR)

stringdef(DUMP_FILE_STR)
stringdef(UNTITLED_ZD_STR)
stringdef(ambiguous_tag_STR)

stringdef(Ctrl__STR)
stringdef(Shift__STR)
stringdef(PRESSANYKEY_STR)
stringdef(Initialising_STR)
stringdef(Debug_STR)

stringdef(FZd_STR)
stringdef(New_width_STR)
stringdef(New_right_margin_position_STR)
stringdef(Zld_one_word_found_STR)
stringdef(Zld_many_words_found_STR)
stringdef(Zld_one_string_found_STR)
stringdef(Zld_many_strings_found_STR)
stringdef(Zld_one_string_hidden_STR)
stringdef(Zld_many_strings_hidden_STR)
stringdef(Zd_unrecognised_word_STR)
stringdef(Zd_unrecognised_words_STR)
stringdef(Zd_word_added_to_user_dict_STR)
stringdef(Zd_words_added_to_user_dict_STR)
stringdef(Zs_of_Zs_STR)
stringdef(Zs_complete_STR)
stringdef(Page_Zd_wait_between_pages_STR)
stringdef(Miss_Page_Chars_STR)
stringdef(All_Pages_Chars_STR)

stringdef(save_edited_file_Zs_STR)
stringdef(save_edited_chart_Zs_STR)

stringdef(Zd_file_edited_in_Zs_are_you_sure_STR)
stringdef(Zd_files_edited_in_Zs_are_you_sure_STR)

stringdef(close_dependent_files_winge_STR)
stringdef(close_dependent_charts_winge_STR)
stringdef(close_dependent_links_winge_STR)

stringdef(load_supporting_winge_STR)
stringdef(name_supporting_winge_STR)

stringdef(Dump_STR)
stringdef(Dumping_STR)
stringdef(Merge_STR)
stringdef(Merging_STR)
stringdef(Anagrams_STR)
#ifndef Anagrams_dialog_STR
#define Anagrams_dialog_STR Anagrams_STR
#endif
stringdef(Subgrams_STR)
#ifndef Subgrams_dialog_STR
#define Subgrams_dialog_STR Subgrams_STR
#endif
stringdef(Opened_user_dictionaries_STR)
stringdef(Insert_word_in_user_dictionary_dialog_STR)
stringdef(Delete_word_from_user_dictionary_dialog_STR)
stringdef(Insert_highlights_dialog_STR)

stringdef(Overwrite_existing_file_STR)
stringdef(Cannot_store_block_STR)

stringdef(help_iconbar)
stringdef(help_dialog_window)
stringdef(help_main_window)
stringdef(help_drag_file_to_insert)
stringdef(help_click_select_to)
stringdef(help_click_adjust_to)
stringdef(help_position_the_caret_in)
stringdef(help_insert_a_reference_to)
stringdef(help_slot)
stringdef(help_colh_inexpression_line)
stringdef(help_colh_inexpression_box)
stringdef(help_top_left_corner)
stringdef(help_col_border)
stringdef(help_drag_row_border)
stringdef(help_double_row_border)
stringdef(help_row_is_page_break)
stringdef(help_row_is_hard_page_break)
stringdef(help_colh_contents_line)
stringdef(help_colh_contents_line_inexpression)
stringdef(help_colh_transfer_to_window)
stringdef(help_colh_edit_in_window)
stringdef(help_drag_column_width)
stringdef(help_drag_right_margin)
stringdef(help_double_right_margin)

stringdef(iconbar_menu_entries)

stringdef(ic_menu_title)

stringdef(iw_menu_title)
stringdef(iw_menu_prefix)

stringdef(pf_menu_title)
stringdef(pf_menu_entries)
stringdef(fs_menu_title)
stringdef(fs_menu_entries_height)
stringdef(fs_menu_entries_width)

stringdef(No_RISC_OS_STR)
stringdef(Zs_printer_driver_STR)

stringdef(fp_only_validation_STR)
stringdef(pointsize_STR)

stringdef(memory_size_Zd_STR)

stringdef(_unable_to_create_new_document_STR)

stringdef(Blocks_STR)
stringdef(Cursor_STR)
stringdef(Edit_STR)
stringdef(Files_STR)
stringdef(Layout_STR)
stringdef(Spell_STR)

stringdef(Mark_block_STR)
stringdef(Clear_markers_STR)
stringdef(Copy_block_STR)
stringdef(Copy_block_to_paste_list_STR)
stringdef(Size_of_paste_list_STR)
stringdef(Move_block_STR)
stringdef(Delete_block_STR)
stringdef(Clear_block_STR)
stringdef(Replicate_down_STR)
stringdef(Replicate_right_STR)
stringdef(Replicate_STR)
stringdef(Sort_STR)
stringdef(Search_STR)
stringdef(Next_match_STR)
stringdef(Previous_match_STR)
stringdef(Set_protection_STR)
stringdef(Clear_protection_STR)
stringdef(Number_X_Text_STR)
stringdef(Snapshot_STR)
stringdef(Word_count_STR)

stringdef(Format_paragraph_STR)
stringdef(FormatBlock_STR)
stringdef(First_column_STR)
stringdef(Last_column_STR)
stringdef(Next_word_STR)
stringdef(Previous_word_STR)
stringdef(Centre_window_STR)
stringdef(Save_position_STR)
stringdef(Restore_position_STR)
stringdef(Swap_position_and_caret_STR)
stringdef(Go_to_slot_STR)
stringdef(Define_key_STR)
stringdef(Define_function_key_STR)
stringdef(Record_macro_file_STR)
#ifndef Record_macro_file_dialog_STR
#define Record_macro_file_dialog_STR Record_macro_file_STR
#endif
stringdef(Do_macro_file_STR)
#ifndef Do_macro_file_dialog_STR
#define Do_macro_file_dialog_STR Do_macro_file_STR
#endif
stringdef(Define_command_STR)

stringdef(Delete_character_STR)
stringdef(Insert_space_STR)

#if 0
stringdef(Insert_character_STR)
#endif

stringdef(Delete_word_STR)
stringdef(Delete_to_end_of_slot_STR)
stringdef(Delete_row_STR)
stringdef(Insert_row_STR)
stringdef(Paste_STR)
stringdef(Insert_STR)
stringdef(Swap_case_STR)
stringdef(Edit_formula_STR)
stringdef(Insert_reference_STR)
stringdef(Split_line_STR)
stringdef(Join_lines_STR)
stringdef(Insert_row_in_column_STR)
stringdef(Delete_row_in_column_STR)
stringdef(Insert_column_STR)
stringdef(Delete_column_STR)
stringdef(Add_column_STR)
stringdef(Insert_page_STR)

stringdef(Load_STR)
stringdef(Save_STR)
stringdef(Save_template_STR)
#ifndef Save_template_dialog_STR
#define Save_template_dialog_STR Save_template_STR
#endif
stringdef(Name_STR)
stringdef(New_window_STR)
stringdef(Short_menus_STR)
stringdef(Options_STR)
stringdef(Colours_STR)
stringdef(Next_file_STR)
stringdef(Previous_file_STR)
stringdef(Top_file_STR)
stringdef(Bottom_file_STR)
stringdef(Recalculate_STR)
stringdef(Auto_recalculation_STR)
stringdef(Auto_chart_recalculation_STR)

stringdef(RepeatCommand_STR)
stringdef(Help_STR)
stringdef(About_STR)
stringdef(Exit_STR)

stringdef(Set_column_width_STR)
stringdef(Set_right_margin_STR)
stringdef(Fix_row_STR)
stringdef(Fix_column_STR)
stringdef(Move_margin_right_STR)
stringdef(Move_margin_left_STR)
stringdef(Auto_width_STR)
stringdef(Link_columns_STR)
stringdef(Left_align_STR)
stringdef(Centre_align_STR)
stringdef(Right_align_STR)
stringdef(LCR_align_STR)
stringdef(Free_align_STR)
stringdef(Decimal_places_STR)
stringdef(Sign_brackets_STR)
stringdef(Sign_minus_STR)
stringdef(Leading_characters_STR)
stringdef(Trailing_characters_STR)
stringdef(Default_format_STR)

stringdef(Print_STR)
stringdef(Set_parameter_STR)
stringdef(Page_layout_STR)
stringdef(Printer_configuration_STR)
stringdef(Microspace_pitch_STR)
stringdef(Edit_printer_driver_STR)
stringdef(Printer_font_STR)
stringdef(Insert_font_STR)
stringdef(Printer_line_spacing_STR)
stringdef(Underline_STR)
stringdef(Bold_STR)
stringdef(Extended_sequence_STR)
stringdef(Italic_STR)
stringdef(Subscript_STR)
stringdef(Superscript_STR)
stringdef(Alternate_font_STR)
stringdef(User_defined_STR)
stringdef(Remove_highlights_STR)
#ifndef Remove_highlights_dialog_STR
#define Remove_highlights_dialog_STR Remove_highlights_STR
#endif
stringdef(Highlight_block_STR)

stringdef(Auto_check_STR)
stringdef(Check_document_STR)
stringdef(Delete_word_from_user_dict_STR)
stringdef(Insert_word_in_user_dict_STR)
stringdef(Display_user_dictionaries_STR)
stringdef(Browse_STR)
stringdef(Dump_dictionary_STR)
stringdef(Merge_file_with_user_dict_STR)
stringdef(Create_user_dictionary_STR)
#ifndef Create_user_dictionary_dialog_STR
#define Create_user_dictionary_dialog_STR Create_user_dictionary_STR
#endif
stringdef(Open_user_dictionary_STR)
#ifndef Open_user_dictionary_dialog_STR
#define Open_user_dictionary_dialog_STR Open_user_dictionary_STR
#endif
stringdef(Close_user_dictionary_STR)
#ifndef Close_user_dictionary_dialog_STR
#define Close_user_dictionary_dialog_STR Close_user_dictionary_STR
#endif
stringdef(Flush_user_dictionary_STR)
stringdef(Pack_user_dictionary_STR)
stringdef(Dictionary_options_STR)
stringdef(Lock_dictionary_STR)
#ifndef Lock_dictionary_dialog_STR
#define Lock_dictionary_dialog_STR Lock_dictionary_STR
#endif
stringdef(Unlock_dictionary_STR)
#ifndef Unlock_dictionary_dialog_STR
#define Unlock_dictionary_dialog_STR Unlock_dictionary_STR
#endif

stringdef(Cursor_up_STR)
stringdef(Cursor_down_STR)
stringdef(Cursor_left_STR)
stringdef(Cursor_right_STR)
stringdef(Top_of_column_STR)
stringdef(Bottom_of_column_STR)
stringdef(Start_of_slot_STR)
stringdef(End_of_slot_STR)
stringdef(Scroll_up_STR)
stringdef(Scroll_down_STR)
stringdef(Scroll_left_STR)
stringdef(Scroll_right_STR)
stringdef(Page_up_STR)
stringdef(Page_down_STR)
stringdef(Page_left_STR)
stringdef(Page_right_STR)
stringdef(Enter_STR)
stringdef(Rubout_STR)
stringdef(Next_column_STR)
stringdef(Previous_column_STR)
stringdef(Escape_STR)
stringdef(Pause_STR)
stringdef(Replace_STR)
stringdef(Next_window_STR)
stringdef(Close_window_STR)

stringdef(Tidy_up_STR)
stringdef(Load_Template_STR)

stringdef(Mark_sheet_STR)
stringdef(New_slot_contents_STR)

/* thousands separator options */

stringdef(thousands_none_STR)
stringdef(thousands_comma_STR)
stringdef(thousands_dot_STR)
stringdef(thousands_space_STR)

/* printer type options */

stringdef(printertype_RISC_OS_STR)
stringdef(printertype_Parallel_STR)
stringdef(printertype_Serial_STR)
stringdef(printertype_Network_STR)
stringdef(printertype_User_STR)

/* function key names */

stringdef(F1_STR)
stringdef(F2_STR)
stringdef(F3_STR)
stringdef(F4_STR)
stringdef(F5_STR)
stringdef(F6_STR)
stringdef(F7_STR)
stringdef(F8_STR)
stringdef(F9_STR)
stringdef(F10_STR)
stringdef(F11_STR)
stringdef(F12_STR)

stringdef(Shift_F1_STR)
stringdef(Shift_F2_STR)
stringdef(Shift_F3_STR)
stringdef(Shift_F4_STR)
stringdef(Shift_F5_STR)
stringdef(Shift_F6_STR)
stringdef(Shift_F7_STR)
stringdef(Shift_F8_STR)
stringdef(Shift_F9_STR)
stringdef(Shift_F10_STR)
stringdef(Shift_F11_STR)
stringdef(Shift_F12_STR)

stringdef(Ctrl_F1_STR)
stringdef(Ctrl_F2_STR)
stringdef(Ctrl_F3_STR)
stringdef(Ctrl_F4_STR)
stringdef(Ctrl_F5_STR)
stringdef(Ctrl_F6_STR)
stringdef(Ctrl_F7_STR)
stringdef(Ctrl_F8_STR)
stringdef(Ctrl_F9_STR)
stringdef(Ctrl_F10_STR)
stringdef(Ctrl_F11_STR)
stringdef(Ctrl_F12_STR)

stringdef(Ctrl_Shift_F1_STR)
stringdef(Ctrl_Shift_F2_STR)
stringdef(Ctrl_Shift_F3_STR)
stringdef(Ctrl_Shift_F4_STR)
stringdef(Ctrl_Shift_F5_STR)
stringdef(Ctrl_Shift_F6_STR)
stringdef(Ctrl_Shift_F7_STR)
stringdef(Ctrl_Shift_F8_STR)
stringdef(Ctrl_Shift_F9_STR)
stringdef(Ctrl_Shift_F10_STR)
stringdef(Ctrl_Shift_F11_STR)
stringdef(Ctrl_Shift_F12_STR)

/* month names */

stringdef(month_January_STR)
stringdef(month_February_STR)
stringdef(month_March_STR)
stringdef(month_April_STR)
stringdef(month_May_STR)
stringdef(month_June_STR)
stringdef(month_July_STR)
stringdef(month_August_STR)
stringdef(month_September_STR)
stringdef(month_October_STR)
stringdef(month_November_STR)
stringdef(month_December_STR)

stringdef(YES_STR)
stringdef(NO_STR)

stringdef(DEFAULTLEADING_STR)
stringdef(DEFAULTTRAILING_STR)
stringdef(DEFAULTTEXTAT_STR)

stringdef(DEFAULTPSERVER_STR)
stringdef(DEFAULTITERLIMIT_STR)
stringdef(DEFAULTITERCOUNT_STR)

stringdef(lf_str)
stringdef(cr_str)
stringdef(crlf_str)
stringdef(lfcr_str)

stringdef(Edit_formula_in_window_STR)

stringdef(SaveChoices_STR)
stringdef(FullScreen_STR)
stringdef(SpreadArray_STR)
stringdef(ChangeMenus_STR)

stringdef(New_chart_STR)
stringdef(Chart_options_STR)
stringdef(Add_to_chart_STR)
stringdef(Remove_from_chart_STR)
stringdef(Edit_chart_STR)
stringdef(Delete_chart_STR)
stringdef(Select_chart_STR)
stringdef(Chart_STR)

stringdef(CustFuncErrStr)
stringdef(PropagatedErrStr)

stringdef(function_menu_title)
stringdef(function_menu_entries)

stringdef(menu_function_database_title)
stringdef(menu_function_date_title)
stringdef(menu_function_financial_title)
stringdef(menu_function_lookup_title)
stringdef(menu_function_string_title)
stringdef(menu_function_complex_title)
stringdef(menu_function_maths_title)
stringdef(menu_function_matrix_title)
stringdef(menu_function_stat_title)
stringdef(menu_function_trig_title)
stringdef(menu_function_misc_title)
stringdef(menu_function_control_title)
stringdef(menu_function_custom_title)
stringdef(menu_function_names_title)
stringdef(Edit_name_STR)
stringdef(formwind_title_STR)
stringdef(menu_function_numinfo_title)
stringdef(menu_function_numinfo_entries)
stringdef(menu_function_numinfo_value_title)
stringdef(menu_function_numinfo_slr_title)
stringdef(menu_function_numinfo_range_title)
stringdef(menu_function_numinfo_name_title)
stringdef(menu_function_textinfo_title)
stringdef(menu_function_textinfo_entries)
stringdef(menu_function_textinfo_title_blank)
stringdef(formwind_menu_title)

stringdef(F_Print_STR)
stringdef(SF_Print_STR)
stringdef(CF_Print_STR)
stringdef(CSF_Print_STR)

stringdef(F_Tab_STR)
stringdef(SF_Tab_STR)
stringdef(CF_Tab_STR)
stringdef(CSF_Tab_STR)

stringdef(F_Insert_STR)
stringdef(SF_Insert_STR)
stringdef(CF_Insert_STR)
stringdef(CSF_Insert_STR)

stringdef(F_Home_STR)
stringdef(SF_Home_STR)
stringdef(CF_Home_STR)
stringdef(CSF_Home_STR)

stringdef(F_Copy_STR)
stringdef(SF_Copy_STR)
stringdef(CF_Copy_STR)
stringdef(CSF_Copy_STR)

stringdef(F_Delete_STR)
stringdef(SF_Delete_STR)
stringdef(CF_Delete_STR)
stringdef(CSF_Delete_STR)

stringdef(F_Backspace_STR)
stringdef(SF_Backspace_STR)
/*stringdef(CF_Backspace_STR)*/
/*stringdef(CSF_Backspace_STR)*/

stringdef(ToNumber_STR)
stringdef(ToText_STR)

stringdef(colh_button_replicd)
stringdef(colh_button_replicr)
stringdef(colh_button_graph)

stringdef(colh_button_save)
stringdef(colh_button_print)

stringdef(colh_button_ljustify)
stringdef(colh_button_cjustify)
stringdef(colh_button_rjustify)
stringdef(colh_button_fjustify)

stringdef(colh_button_font)
stringdef(colh_button_bold)
stringdef(colh_button_italic)
stringdef(colh_button_underlined)
stringdef(colh_button_superscript)
stringdef(colh_button_subscript)
stringdef(colh_button_leadtrail)
stringdef(colh_button_decplaces)

stringdef(colh_button_search)
stringdef(colh_button_sort)
stringdef(colh_button_spellcheck)

stringdef(colh_button_copy)
stringdef(colh_button_delete)
stringdef(colh_button_paste)

stringdef(colh_button_totext)
stringdef(colh_button_tonumber)
stringdef(colh_button_edit_ok)
stringdef(colh_button_edit_cancel)

stringdef(colh_button_formatblock)
stringdef(colh_button_cmd_record)
stringdef(colh_button_cmd_exec)

#endif /* __strings_h */

/* end of strings.h */
