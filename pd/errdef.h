/* errdef.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

#ifndef __errdef_h
#define __errdef_h

/*
exported functions
*/

extern void
messagef(
    _In_z_ _Printf_format_string_ PC_U8Z format,
    /**/        ...);

extern void
message_output(
    _In_z_      PC_U8Z buffer);

/*ncr*/
extern BOOL
reperr(
    STATUS errornumber,
    _In_opt_z_  PC_U8Z text,
    /**/        ...);

extern PC_U8
reperr_getstr(
    STATUS errornumber);

/*ncr*/
extern BOOL
reperr_null(
    STATUS errornumber);

/*ncr*/
extern BOOL
reperr_not_installed(
    STATUS errornumber);

/*ncr*/
extern BOOL
rep_fserr(
    _In_z_      PC_U8Z str);

/*ncr*/
extern BOOL
reperr_kernel_oserror(
    _In_        _kernel_oserror * const err);

/* error definition */

#define ERR_OUTPUTSTRING 1

#define MAIN_ERR_BASE (0)

/* NB some are reserved for STATUS_xxx codes */

#define ERR_BAD_NAME            MAIN_ERR_BASE - 10
#define ERR_NOTFOUND            MAIN_ERR_BASE - 11 /* file not found */
#define ERR_BAD_SELECTION       MAIN_ERR_BASE - 12
#define ERR_BAD_PARM            MAIN_ERR_BASE - 13
#define ERR_EDITINGEXP          MAIN_ERR_BASE - 14
#define ERR_CANNOTOPEN          MAIN_ERR_BASE - 15
#define ERR_CANNOTREAD          MAIN_ERR_BASE - 16
#define ERR_CANNOTWRITE         MAIN_ERR_BASE - 17
#define ERR_CANNOTCLOSE         MAIN_ERR_BASE - 18
#define ERR_CHART_ALREADY_LOADED MAIN_ERR_BASE - 19
#define ERR_BAD_HAT             MAIN_ERR_BASE - 20
#define ERR_NOLISTFILE          MAIN_ERR_BASE - 21
#define ERR_LOOP                MAIN_ERR_BASE - 22
#define ERR_ENDOFLIST           MAIN_ERR_BASE - 23
#define ERR_CANNOTBUFFER        MAIN_ERR_BASE - 24
#define ERR_BAD_OPTION          MAIN_ERR_BASE - 25
#define ERR_NOBLOCK             MAIN_ERR_BASE - 26
#define ERR_ESCAPE              MAIN_ERR_BASE - 27
#define ERR_BAD_COL             MAIN_ERR_BASE - 28
#define ERR_OLDEST_LOST         MAIN_ERR_BASE - 29
#define ERR_ALREADY_IN_DIALOG   MAIN_ERR_BASE - 30
#define ERR_BAD_EXPRESSION      MAIN_ERR_BASE - 31
#define ERR_BAD_MARKER          MAIN_ERR_BASE - 32
#define ERR_NO_DRIVER           MAIN_ERR_BASE - 33
#define ERR_NO_MICRO            MAIN_ERR_BASE - 34
#define ERR_GENFAIL             MAIN_ERR_BASE - 35
#define ERR_OVERLAP             MAIN_ERR_BASE - 36
#define ERR_CTRL_CHARS          MAIN_ERR_BASE - 37
#define ERR_CANNOTINSERT        MAIN_ERR_BASE - 38
#define ERR_LOTUS               MAIN_ERR_BASE - 39
#define ERR_NOPAGES             MAIN_ERR_BASE - 40
#define ERR_SPELL               MAIN_ERR_BASE - 41
#define ERR_WORDEXISTS          MAIN_ERR_BASE - 42
#define ERR_PRINTER             MAIN_ERR_BASE - 43
#define ERR_NOTTABFILE          MAIN_ERR_BASE - 44
#define ERR_LINES_SPLIT         MAIN_ERR_BASE - 45
#define ERR_NOTREE              MAIN_ERR_BASE - 46
#define ERR_BAD_STRING          MAIN_ERR_BASE - 47
#define ERR_BAD_CELL            MAIN_ERR_BASE - 48
#define ERR_BAD_RANGE           MAIN_ERR_BASE - 49
#define ERR_SHEET               MAIN_ERR_BASE - 50
#define ERR_PROTECTED           MAIN_ERR_BASE - 51
#define ERR_AWAITRECALC         MAIN_ERR_BASE - 52
#define ERR_BADDRAWFILE         MAIN_ERR_BASE - 53
#define ERR_BADFONTSIZE         MAIN_ERR_BASE - 54
#define ERR_BADLINESPACE        MAIN_ERR_BASE - 55
#define ERR_CANTWRAP            MAIN_ERR_BASE - 56
#define ERR_BADDRAWSCALE        MAIN_ERR_BASE - 57
#define ERR_PRINT_WONT_OPEN     MAIN_ERR_BASE - 58
#define ERR_NORISCOSPRINTER     MAIN_ERR_BASE - 59
#define ERR_FONTY               MAIN_ERR_BASE - 60
#define ERR_ALREADYDUMPING      MAIN_ERR_BASE - 61
#define ERR_ALREADYMERGING      MAIN_ERR_BASE - 62
#define ERR_ALREADYANAGRAMS     MAIN_ERR_BASE - 63
#define ERR_ALREADYSUBGRAMS     MAIN_ERR_BASE - 64
#define ERR_was_CANTINSTALL     MAIN_ERR_BASE - 65
#define ERR_was_DEMO            MAIN_ERR_BASE - 66
#define ERR_NOROOMFORDBOX       MAIN_ERR_BASE - 67
#define ERR_NOTINDESKTOP        MAIN_ERR_BASE - 68
#define ERR_BADPRINTSCALE       MAIN_ERR_BASE - 69
#define ERR_NOBLOCKINDOC        MAIN_ERR_BASE - 70
#define ERR_CANTSAVEPASTEBLOCK  MAIN_ERR_BASE - 71
#define ERR_CANTLOADPASTEBLOCK  MAIN_ERR_BASE - 72
#define ERR_CANTSAVETOITSELF    MAIN_ERR_BASE - 73
#define ERR_BAD_LANGUAGE        MAIN_ERR_BASE - 74
#define ERR_NOTINSTALLED        MAIN_ERR_BASE - 75
#define ERR_NOSELCHART          MAIN_ERR_BASE - 76
#define ERR_TOOMANYDOCS         MAIN_ERR_BASE - 77
#define ERR_LINETOOLONG         MAIN_ERR_BASE - 78
#define ERR_SYSTEMFONTSELECTED  MAIN_ERR_BASE - 79
#define ERR_CHARTIMPORTNEEDSSAVEDDOC    MAIN_ERR_BASE - 80
#define ERR_CHARTNONUMERICDATA  MAIN_ERR_BASE - 81
#define ERR_HELP_URL_FAILURE    MAIN_ERR_BASE - 82
#define ERR_SUPPORTING_DOC_NOT_FOUND    MAIN_ERR_BASE - 83
#define ERR_CANT_LOAD_FILETYPE  MAIN_ERR_BASE - 84
#define ERR_NO_ALTERNATE_COMMAND        MAIN_ERR_BASE - 85

#define MAIN_ERR_END           (MAIN_ERR_BASE - 86)

/* module errors start here at intervals of increment */
#ifndef MODULE_ERR_BASE
#define MODULE_ERR_BASE      (-1000)
#endif
#ifndef MODULE_ERR_INCREMENT
#define MODULE_ERR_INCREMENT 1000
#endif

#define APP_ERRLIST_DEF \
    errorstring(-1,  "STATUS_ERROR_RQ") \
    errorstring(-2,  "STATUS_FAIL") \
    errorstring(-3,  "Memory full") \
    errorstring(-4,  "Cancel") \
    errorstring(-10, "Bad name") \
    errorstring(-11, "File not found") \
    errorstring(-12, "Bad selection") \
    errorstring(-13, "Bad parameter") \
    errorstring(-14, "Editing expression") \
    errorstring(-15, "Cannot open file") \
    errorstring(-16, "Cannot read file") \
    errorstring(-17, "Cannot write to file") \
    errorstring(-18, "Cannot close file") \
    errorstring(-19, "'%s' already loaded") \
    errorstring(-20, "Bad ^ field") \
    errorstring(-21, "No list file") \
    errorstring(-22, "Loop") \
    errorstring(-23, "End of list") \
    errorstring(-24, "Cannot buffer file") \
    errorstring(-25, "Bad option") \
    errorstring(-26, "No marked block") \
    errorstring(-27, "Escape") \
    errorstring(-28, "Bad column") \
    errorstring(-29, "Oldest position lost") \
    errorstring(-30, "Please complete the existing dialogue box") \
    errorstring(-31, "Bad expression") \
    errorstring(-32, "Bad marker") \
    errorstring(-33, "No driver loaded") \
    errorstring(-34, "Driver does not support microspacing") \
    errorstring(-35, "General failure") \
    errorstring(-36, "Overlap") \
    errorstring(-37, "Control characters ignored") \
    errorstring(-38, "Cannot insert this file format") \
    errorstring(-39, "Lotus:") \
    errorstring(-40, "No pages") \
    errorstring(-41, "Spell:") \
    errorstring(-42, "Word exists") \
    errorstring(-43, "Printer:") \
    errorstring(-44, "'%s' is not an plain Text file") \
    errorstring(-45, "Lines split:") \
    errorstring(-46, "No room: column recalculation forced") \
    errorstring(-47, "Bad string") \
    errorstring(-48, "Bad cell") \
    errorstring(-49, "Bad range") \
    errorstring(-50, "ViewSheet:") \
    errorstring(-51, "Protected cell") \
    errorstring(-52, "Awaiting recalculation") \
    errorstring(-53, "Bad Draw file") \
    errorstring(-54, "Bad font size") \
    errorstring(-55, "Bad line spacing") \
    errorstring(-56, "Wrap option switched off") \
    errorstring(-57, "Bad Draw file scale factor") \
    errorstring(-58, "Printer in use") \
    errorstring(-59, "No RISC OS printer driver") \
    errorstring(-60, "Font manager:") \
    errorstring(-61, "Already dumping") \
    errorstring(-62, "Already merging") \
    errorstring(-63, "Already doing anagrams") \
    errorstring(-64, "Already doing subgrams") \
    errorstring(-67, "Memory full: unable to create dialogue box") \
    errorstring(-68, "Not enough memory, or not in *Desktop world") \
    errorstring(-69, "Bad print scale") \
    errorstring(-70, "No marked block in this document") \
    errorstring(-71, "Memory full: unable to store block to paste list") \
    errorstring(-72, "Memory full: unable to recover block from paste list") \
    errorstring(-73, "Cannot save an entire file from PipeDream into itself") \
    errorstring(-74, "Bad language") \
    errorstring(-75, "Not installed") \
    errorstring(-76, "No selected chart") \
    errorstring(-77, "Too many documents") \
    errorstring(-78, "Line too long") \
    errorstring(-79, "System font selected") \
    errorstring(-80, "Charts can only be imported into documents that have been saved") \
    errorstring(-81, "No numeric data in marked block") \
    errorstring(-82, "No browser available to access URL") \
    errorstring(-83, "Supporting document '%s' not found") \
    errorstring(-84, "Can't load this type of file") \
    errorstring(-85, "No alternate command for this toolbar button") \


#endif /* __errdef_h */

/* end of errdef.h */
