/* spell.h */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* External header file for spellcheck */

/* MRJC May 1988 */

#ifndef __spell_h
#define __spell_h

/*
exported functions
*/

extern S32
spell_addword(
    S32 dict,
    char *word);

extern S32
spell_checkword(
    S32 dict,
    char *word);

extern S32
spell_close(
    S32 dict);

extern S32
spell_createdict(
    char *name,
    char *def_name);

extern S32
spell_deleteword(
    S32 dict,
    char *word);

extern S32
spell_flush(
    S32 dict);

extern S32
spell_iswordc(
    S32 dict,
    S32 ch);

extern S32
spell_load(
    S32 dict);

extern S32
spell_nextword(
    S32 dict,
    char *wordout,
    char *wordin,
    char *mask,
    P_S32 brkflg);

extern S32
spell_opendict(
    P_U8 name,
    P_U8 def_name,
    char **copy_right);

extern S32
spell_pack(
    S32 olddict,
    S32 newdict);

extern S32
spell_prevword(
    S32 dict,
    char *wordin,
    char *wordout,
    char *mask,
    P_S32 brkflg);

extern S32
spell_setoptions(
    S32 dict,
    S32 optionset,
    S32 optionmask);

extern void
spell_stats(
    P_S32 cblocks,
    P_S32 largest,
    P_S32 totalmem);

extern S32
spell_strnicmp(
    S32 dict,
    PC_U8 word1,
    PC_U8 word2,
    S32 len);

extern S32
spell_tolower(
    S32 dict,
    S32 ch);

extern S32
spell_toupper(
    S32 dict,
    S32 ch);

extern S32
spell_unlock(
    S32 dict);

extern S32
spell_valid_1(
    S32 dict,
    S32 ch);

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
