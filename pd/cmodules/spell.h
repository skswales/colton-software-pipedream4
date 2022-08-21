/* spell.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* External header file for spellcheck */

/* MRJC May 1988 */

#ifndef __spell_h
#define __spell_h

typedef S32 DICT_NUMBER;

/*
exported functions
*/

_Check_return_
extern STATUS
spell_addword(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      const char *word);

_Check_return_
extern STATUS
spell_checkword(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      const char *word);

_Check_return_
extern STATUS
spell_close(
    _InVal_     DICT_NUMBER dict_number);

_Check_return_
extern STATUS
spell_createdict(
    _In_z_      PCTSTR name,
    _In_z_      PCTSTR def_name);

_Check_return_
extern STATUS
spell_deleteword(
    _InVal_     DICT_NUMBER dict_number,
    _In_z_      const char *word);

_Check_return_
extern STATUS
spell_flush(
    _InVal_     DICT_NUMBER dict_number);

_Check_return_
extern STATUS
spell_iswordc(
    _InVal_     DICT_NUMBER dict_number,
    S32 ch);

_Check_return_
extern STATUS
spell_load(
    _InVal_     DICT_NUMBER dict_number);

_Check_return_
extern STATUS
spell_nextword(
    _InVal_     DICT_NUMBER dict_number,
    char *wordout,
    _In_z_      const char *wordin,
    _In_opt_z_  const char *mask,
    _InoutRef_  P_S32 brkflg);

_Check_return_
extern STATUS
spell_opendict(
    _In_z_      PCTSTR name,
    _Out_opt_   char **copy_right);

_Check_return_
extern STATUS
spell_pack(
    _InVal_     DICT_NUMBER old_dict_number,
    _InVal_     DICT_NUMBER new_dict_number);

_Check_return_
extern STATUS
spell_prevword(
    _InVal_     DICT_NUMBER dict_number,
    char *wordout,
    _In_z_      const char *wordin,
    _In_opt_z_  const char *mask,
    _InoutRef_  P_S32 brkflg);

_Check_return_
extern STATUS
spell_setoptions(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     U32 optionset,
    _InVal_     U32 optionmask);

extern void
spell_stats(
    _OutRef_    P_S32 cblocks,
    _OutRef_    P_S32 largest,
    _OutRef_    P_S32 totalmem);

_Check_return_
extern S32
spell_strnicmp(
    _InVal_     DICT_NUMBER dict_number,
    PC_U8 word1,
    PC_U8 word2,
    _InVal_     U32 len);

_Check_return_
extern STATUS
spell_tolower(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch);

_Check_return_
extern STATUS
spell_toupper(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch);

_Check_return_
extern STATUS
spell_unlock(
    _InVal_     DICT_NUMBER dict_number);

_Check_return_
extern STATUS
spell_valid_1(
    _InVal_     DICT_NUMBER dict_number,
    _InVal_     S32 ch);

/*
wildcard characters
*/

#define SPELL_WILD_SINGLE   '?'
#define SPELL_WILD_MULTIPLE '*'

/*
dictionary flags
*/

#define DICT_WRITEINDEX 0x80
#define DICT_READONLY   0x40

/*
maximum length of word (MAX_CHAR - 1 + 2)
*/

#define MAX_WORD 65

/*
error definition
*/

#define SPELL_ERRLIST_DEF \
    errorstring(SPELL_ERR_DICTFULL,      "Spell: Too many dictionaries open") \
    errorstring(SPELL_ERR_EXISTS,        "Spell: File already exists") \
    errorstring(SPELL_ERR_FILE,          "Spell: Filing system error") \
    errorstring(SPELL_ERR_CANTOPEN,      "Spell: Can't open dictionary") \
    errorstring(SPELL_ERR_BADDICT,       "Spell: Bad dictionary") \
    errorstring(SPELL_ERR_BADWORD,       "Spell: Bad word") \
    errorstring(SPELL_ERR_READONLY,      "Spell: Dictionary read only") \
    errorstring(SPELL_ERR_CANTCLOSE,     "Spell: Can't close dictionary") \
    errorstring(SPELL_ERR_NOTIMP,        "Spell: Not implemented") \
    errorstring(SPELL_ERR_NOTFOUND,      "Spell: Word not found") \
    errorstring(SPELL_ERR_ESCAPE,        "Spell: Escape") \
    errorstring(SPELL_ERR_CANTWRITE,     "Spell: Can't update dictionary") \
    errorstring(SPELL_ERR_CANTREAD,      "Spell: Can't read dictionary") \
    errorstring(SPELL_ERR_CANTENLARGE,   "Spell: Can't enlarge dictionary") \
    errorstring(SPELL_ERR_BADDEFFILE,    "Spell: Bad dictionary definition file") \
    errorstring(SPELL_ERR_DEFCHARERR,    "Spell: Bad character in dictionary definition file") \
    errorstring(SPELL_ERR_CANTOPENDEFN,  "Spell: Can't open dictionary definition file") \
    errorstring(SPELL_ERR_CANTCLOSEDEFN, "Spell: Can't close dictionary definition file")

/*
error definition
*/

#define SPELL_ERR_BASE  (-1000)

#define SPELL_ERR(n)    (SPELL_ERR_BASE - (n))

#define SPELL_ERR_DICTFULL      SPELL_ERR(0)
#define SPELL_ERR_EXISTS        SPELL_ERR(1)
#define SPELL_ERR_FILE          SPELL_ERR(2)
#define SPELL_ERR_CANTOPEN      SPELL_ERR(3)
#define SPELL_ERR_BADDICT       SPELL_ERR(4)
#define SPELL_ERR_spare_1005    SPELL_ERR(5)
#define SPELL_ERR_BADWORD       SPELL_ERR(6)
#define SPELL_ERR_READONLY      SPELL_ERR(7)
#define SPELL_ERR_CANTCLOSE     SPELL_ERR(8)
#define SPELL_ERR_NOTIMP        SPELL_ERR(9)
#define SPELL_ERR_NOTFOUND      SPELL_ERR(10)
#define SPELL_ERR_ESCAPE        SPELL_ERR(11)
#define SPELL_ERR_CANTWRITE     SPELL_ERR(12)
#define SPELL_ERR_CANTREAD      SPELL_ERR(13)
#define SPELL_ERR_CANTENLARGE   SPELL_ERR(14)
#define SPELL_ERR_BADDEFFILE    SPELL_ERR(15)
#define SPELL_ERR_DEFCHARERR    SPELL_ERR(16)
#define SPELL_ERR_CANTOPENDEFN  SPELL_ERR(17)
#define SPELL_ERR_CANTCLOSEDEFN SPELL_ERR(18)

#define SPELL_ERR_END           SPELL_ERR(19)

#endif /* __spell_h */

/* end of spell.h */
