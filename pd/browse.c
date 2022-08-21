/* browse.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Browse through spell dictionary etc. */

/* RJM Jun 1988 */

#include "common/gflags.h"

#include "datafmt.h"

#include "cmodules/spell.h"

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

#ifndef __cs_dbox_h
#include "cs-dbox.h"
#endif

#ifndef __cs_template_h
#include "cs-template.h"
#endif

#ifndef __akbd_h
#include "akbd.h"
#endif

#ifndef          __wm_event_h
#include "cmodules/wm_event.h"
#endif

#include "riscos_x.h"
#include "ridialog.h"

typedef struct
{
    dbox d;
    DICT_NUMBER dict;

    S32 res;
    S32 sofar;
    BOOL wants_nulls;
    BOOL stopped;

    char (*words) [MAX_WORD+1];
}
merge_dump_strukt;

#define MDS_INIT { NULL, -1,  0, 0, FALSE, FALSE,  NULL }

/*
internal functions
*/

static S32
close_dict_always(
    _InVal_     DICT_NUMBER dict);

static BOOL
err_open_master_dict(void);

static S32
get_and_display_words(
    BOOL fillall,
    BOOL iswild,
    char words[BROWSE_DEPTH][MAX_WORD+1],
    char *wild_string,
    char *template,
    char *wild_str,
    _InVal_     DICT_NUMBER dict,
    dbox d,
    BOOL *was_spell_errp);

static P_U8
get_word_from_line(
    _InVal_     DICT_NUMBER dict,
    P_U8 array /*out*/,
    S32 stt_offset,
    P_S32 found_offsetp);

static BOOL
not_in_user_dicts_or_list(
    const char *word);

static S32
open_master_dict(void);

#define BROWSE_CENTRE (BROWSE_DEPTH/2 - 1)

static merge_dump_strukt anagram_mds    = MDS_INIT;
static merge_dump_strukt browse_mds     = MDS_INIT;
static merge_dump_strukt dumpdict_mds   = MDS_INIT;
static merge_dump_strukt mergedict_mds  = MDS_INIT;

/* be generous as to what letters one allows into the spelling buffers */
/* - fault foreign words at adding stage */

/* is character valid as the first in a word? */

static S32
browse_valid_1(
    _InVal_     DICT_NUMBER dict,
    char ch)
{
    return(spell_valid_1(dict, ch)  ||  isalpha(ch));
}

/* SKS believes even with new spelling checker one should allow anything that is
 * isalpha() in the current locale to be treated as part of a word so we can at least
 * add silly words that aren't quite valid to local misspellings list or replace them
 * wholly in a check/browse.
*/

static S32
browse_iswordc(
    _InVal_     DICT_NUMBER dict,
    char ch)
{
    return(spell_iswordc(dict, ch)  ||  isalpha(ch));
}

/******************************************************************************
*
* determine whether word is worth having another bash at for spelling
*
******************************************************************************/

static BOOL
worth_trying_again(
    _InVal_     DICT_NUMBER dict,
    P_U8 array /*inout*/)
{
    P_U8 ptr;
    BOOL res = FALSE;

    /* english: try stripping off trailing ' or 's */
    ptr = array + strlen(array);
    if( (*--ptr == '\'')  ||
        ((spell_tolower(dict, *ptr) == 's')  &&  (*--ptr == '\'')))
    {
        *ptr = '\0';
        res = TRUE;
    }

    return(res);
}

/******************************************************************************
*
* check word at current position in linbuf
*
******************************************************************************/

extern BOOL
check_word_wanted(void)
{
    if((d_auto[0].option != 'Y')  ||  xf_inexpression  ||  !slot_in_buffer)
        return(FALSE);

    if(err_open_master_dict())
    {
        d_auto[0].option = 'N';
        return(FALSE);
    }

    return(TRUE);
}

extern void
check_word(void)
{
    char array[LIN_BUFSIZ];
    S32 res, dict;
    BOOL tried_again;

    if(!check_word_wanted())
        return;

    dict = master_dictionary;

    if(get_word_from_line(dict, array, lecpos, NULL))
    {
        tried_again = FALSE;

    TRY_AGAIN:

        res = spell_checkword(dict, array);

        if(res == create_error(SPELL_ERR_BADWORD))
            res = 0;

        if(res < 0)
            reperr_module(create_error(ERR_SPELL), res);
        else if(!res  &&  not_in_user_dicts_or_list(array))
        {
            if(!tried_again)
                if(worth_trying_again(dict, array))
                {
                    tried_again = TRUE;
                    goto TRY_AGAIN;
                }

            bleep();
        }
    }
}

static S32
close_dict(
    _InVal_     DICT_NUMBER dict)
{
    return( ((dict == dumpdict_mds.dict)   ||
             (dict == mergedict_mds.dict)  ||
             (dict == anagram_mds.dict)    )
                    ? create_error(SPELL_ERR_CANTCLOSE)
                    : close_dict_always(dict)
          );
}

_Check_return_
static STATUS
close_dict_always(
    _InVal_     DICT_NUMBER dict)
{
    STATUS res;

#if 1
    /* SKS after 4.12 26mar92 - treat master dictionary like user dictionaries */
    if(master_dictionary == dict)
        master_dictionary = -1;
#endif

    res = spell_close(dict);

    delete_from_list(&first_user_dict, (S32) dict);

    most_recent = -1;

    return(res);
}

/******************************************************************************
*
*  close all of the user dictionaries
*
******************************************************************************/

extern void
close_user_dictionaries(
    BOOL force)
{
    if( force  ||
        (mergedict_mds.dict == -1  &&  anagram_mds.dict == -1  &&  dumpdict_mds.dict == -1))
    {
        PC_LIST lptr;

        while((lptr = first_in_list(&first_user_dict)) != NULL)
        {
            STATUS res = close_dict_always((S32) lptr->key);

            if(status_fail(res))
            {
                reperr_module(create_error(ERR_SPELL), res);
                been_error = FALSE;
            }
        }
    }
}

/******************************************************************************
*
*  flush all of the user dictionaries
*
******************************************************************************/

extern void
flush_user_dictionaries(void);
extern void
flush_user_dictionaries(void)
{
    PC_LIST lptr;

    for(lptr = first_in_list(&first_user_dict);
        lptr;
        lptr = next_in_list(&first_user_dict))
    {
        STATUS res = spell_flush((S32) lptr->key);

        if(status_fail(res))
        {
            reperr_module(create_error(ERR_SPELL), res);
            been_error = FALSE;
        }
    }
}

/******************************************************************************
*
*  compile the wild string - returning whether it is wild
*  all characters for string MUST be valid for given dict
*
******************************************************************************/

static S32
compile_wild_string(
    _InVal_     DICT_NUMBER dict,
    char *to,
    const char *from)
{
    const char *ptr = from;
    S32 iswild = 0;
    S32 ch;

    /* get word template from wild_string */
    if(!str_isblank(ptr))
        while((ch = *ptr++) != '\0')
        {
            if(ch == '^')
            {
                ch = *ptr++;

                switch(ch)
                {
                case '\0':
                    --ptr;        /* point at 0 again */
                    break;

                case '#':
                    ch = SPELL_WILD_MULTIPLE;

                    /* deliberate drop thru */

                case '?':
                    assert('?' == SPELL_WILD_SINGLE);
                    iswild = TRUE;

                    /* deliberate drop thru */

                default:
                    *to++ = (char) ch;
                    break;
                }
            }
            else if(((ptr == from) ? spell_valid_1 : spell_iswordc) (dict, ch))
                *to++ = spell_tolower(dict, ch);
            else
                return(create_error(SPELL_ERR_BADWORD));
        }

    *to = '\0';
    return(iswild);
}

/******************************************************************************
*
*  delete the words on the mis-spellings list
*
******************************************************************************/

extern void
del_spellings(void)
{
    delete_list(&first_spell);
}

/******************************************************************************
*
* return dictionary number from list of dictionaries
* if dictionary not open, open it iff create is TRUE
*
******************************************************************************/

_Check_return_
extern STATUS
dict_number(
    const char * name,
    BOOL create)
{
    PC_LIST lptr;
    DICT_NUMBER dict;
    S32 res;
    char *leaf;
    char fulldictname[BUF_MAX_PATHSTRING];

    trace_1(TRACE_APP_PD4, "dict_number(%s)", trace_string(name));

    if(str_isblank(name))
    {
        most_recent = -1;
        return(create_error(ERR_BAD_NAME));
    }

    leaf = file_leafname(name);

    /* is it already on list? */

    for(lptr = first_in_list(&first_user_dict);
        lptr;
        lptr = next_in_list(&first_user_dict))
    {
        if(0 == _stricmp(leaf, file_leafname((char *) lptr->value)))
            return(most_recent = (S32) lptr->key);
    }

    /* not on list so open it possibly (may even be relative to document) */
    dict = create_error(SPELL_ERR_CANTOPEN);
    if(create)
        if(add_path_or_relative_using_dir(fulldictname, elemof32(fulldictname), name, TRUE, DICTS_SUBDIR_STR))
            if((dict = spell_opendict(fulldictname, NULL)) >= 0)
            {
                if(status_fail(res = add_to_list(&first_user_dict, dict, fulldictname)))
                {
                    status_consume(spell_close(dict));
                    dict = (S32) res;
                }
                else
                    return(most_recent = dict);
            }

    most_recent = -1;
    return(dict);
}

/******************************************************************************
*
*  get the word on the line currently, or previous one
*  this allows valid chars and some that might not be
*
******************************************************************************/

static P_U8
get_word_from_line(
    _InVal_     DICT_NUMBER dict,
    P_U8 array /*out*/,
    S32 stt_offset,
    P_S32 found_offsetp)
{
    P_U8 to = array;
    S32 len;
    PC_U8 src, ptr;
    S32 found_offset = stt_offset;

    trace_2(TRACE_APP_PD4, "get_word_from_line(): linbuf '%s', stt_offset = %d", linbuf, stt_offset);

    if(is_current_document()  &&  !xf_inexpression  &&  slot_in_buffer)
    {
        src = linbuf;
        len = strlen(src);
        ptr = src + MIN(len, stt_offset);

        /* goto start of this or last word */

        /* skip back until a word is hit */
        while((ptr > src)  &&  !browse_iswordc(dict, *ptr))
            --ptr;

        /* skip back until a valid word start is hit */
        while((ptr > src)  &&  browse_iswordc(dict, *(ptr-1)))
            --ptr;

        /* words must start with a letter from the current character set
         * as we don't know which dictionary will be used
        */
        while(browse_iswordc(dict, *ptr)  &&  !browse_valid_1(dict, *ptr))
            ++ptr;

        if(browse_valid_1(dict, *ptr))
        {
            found_offset = ptr - src;

            while(browse_iswordc(dict, *ptr)  &&  ((to - array) < MAX_WORD))
                *to++ = (S32) *ptr++;
        }
    }

    *to = '\0';

    if(found_offsetp)
        *found_offsetp = found_offset;

    trace_2(TRACE_APP_PD4, "get_word_from_line returns '%s', found_offset = %d",
            trace_string((*array != '\0') ? array : NULL), found_offsetp ? *found_offsetp : -1);

    return((*array != '\0') ? array : NULL);
}

/******************************************************************************
*
* put dictionary name of most recently used dictionary in the field
*
******************************************************************************/

static BOOL
insert_most_recent(
    char **field)
{
    if(most_recent >= 0)
        return(mystr_set(field, file_leafname(search_list(&first_user_dict, (S32) most_recent)->value)));

    return(mystr_set(field, USERDICT_STR));
}

_Check_return_
static STATUS
open_appropriate_dict(
    const DIALOG *dptr)
{
    return(    ((dptr->option == 'N')  ||  str_isblank(dptr->textfield))
                    ? open_master_dict()
                    : dict_number(dptr->textfield, TRUE)
            );
}

/******************************************************************************
*
*  open master dictionary
*
******************************************************************************/

static BOOL
err_open_master_dict(void)
{
    S32 res = open_master_dict();

    if(res < 0)
        return(!reperr_module(create_error(ERR_SPELL), res));

    return(FALSE);
}

_Check_return_
static STATUS
open_master_dict(void)
{
    PC_U8 name;

    if(master_dictionary < 0)
    {
        name = MASTERDICT_STR;

        /* SKS after 4.12 26mar92 - treat master dictionary just like user dictionaries */
        master_dictionary = dict_number(name, TRUE);

        /* don't hint that this is the one to be molested */
        most_recent = -1;
    }

    return(master_dictionary);
}

/******************************************************************************
*
*  create user dictionary
*
******************************************************************************/

extern void
CreateUserDict_fn(void)
{
    char buffer[BUF_MAX_PATHSTRING];
    char buffer2[BUF_MAX_PATHSTRING];
    char *name;
    char *language;
    S32 res;
  /*os_error *err;*/

    if(!dialog_box_start())
        return;

    /* Throw away any already existing language_list
     * then scan the dictionary definitions directory
     * and (re)build the language_list.
     * ridialog will try to retain the user's last chosen language.
    */
    delete_list(&language_list);

    /* enumerate all files in subdirectory DICTDEFNS_SUBDIR_STR found in the directories listed by the PipeDream$Path variable */

    status_assert(enumerate_dir_to_list(&language_list, DICTDEFNS_SUBDIR_STR, FILETYPE_UNDETERMINED)); /* errors are ignored */

    if(!mystr_set(&d_user_create[0].textfield, USERDICT_STR))
        return;

    /* leave d_user_create[1].textfield alone */

    while(dialog_box(D_USER_CREATE))
    {
        name     = d_user_create[0].textfield;
        language = d_user_create[1].textfield;

        if(str_isblank(name))
        {
            reperr_null(create_error(ERR_BAD_NAME));
            if(!dialog_box_can_retry())
                break;
            (void) mystr_set(&d_user_create[0].textfield, USERDICT_STR);
            continue;
        }

        /* try to create dictionary in a subdirectory of <Choices$Write>.PipeDream */
        name = add_choices_write_prefix_to_name_using_dir(buffer, elemof32(buffer), name, DICTS_SUBDIR_STR);

        if(str_isblank(language))
        {
            reperr_null(create_error(ERR_BAD_LANGUAGE));
            if(!dialog_box_can_retry())
                break;
            (void) mystr_set(&d_user_create[1].textfield, DEFAULT_DICTDEFN_FILE_STR);
            continue;
        }

        /* Add prefix '<PipeDream$Dir>.DictDefn.' to language */

        if((res = add_path_using_dir(buffer2, elemof32(buffer2), language, DICTDEFNS_SUBDIR_STR)) <= 0)
        {
            reperr_null(res);
            if(!dialog_box_can_retry())
                break;
            continue;
        }
        language = buffer2;

        trace_2(TRACE_APP_PD4, "name='%s', language='%s'", name, language);

        if(TRUE /*checkoverwrite(name)*/)
        {
            res = spell_createdict(name, language);

            if(res >= 0)
                res = dict_number(name, TRUE);

            if(res < 0)
            {
                reperr_module(create_error(ERR_SPELL), res);
                if(!dialog_box_can_retry())
                    break;
                continue;
            }
        }

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* open user dictionary
*
******************************************************************************/

extern void
OpenUserDict_fn(void)
{
    char *name;
    S32 res;

    if(!init_dialog_box(D_USER_OPEN))
        return;

    if(!dialog_box_start())
        return;

    while(dialog_box(D_USER_OPEN))
    {
        name = d_user_open[0].textfield;

        if(str_isblank(name))
        {
            reperr_null(create_error(ERR_BAD_NAME));
            if(!dialog_box_can_retry())
                break;
            (void) mystr_set(&d_user_open[0].textfield, USERDICT_STR);
            continue;
        }

        /* open it, if not already open */
        res = dict_number(name, TRUE);

        if(res < 0)
        {
            reperr_module(create_error(ERR_SPELL), res);
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* close user dictionary
*
******************************************************************************/

extern void
CloseUserDict_fn(void)
{
    char *name;
    S32 res;

    if(!init_dialog_box(D_USER_CLOSE))
        return;

    if(!dialog_box_start())
        return;

    if(!insert_most_recent(&d_user_close[0].textfield))
        return;

    while(dialog_box(D_USER_CLOSE))
    {
        name = d_user_close[0].textfield;

        if(str_isblank(name))
        {
            reperr_null(create_error(ERR_BAD_NAME));
            if(!dialog_box_can_retry())
                break;
            (void) mystr_set(&d_user_close[0].textfield, USERDICT_STR);
            continue;
            }

        /* check it is open already, but not allowing it to be opened */
        res = dict_number(name, FALSE);

        if(res >= 0)
            res = close_dict(res);
        else
            res = 0;    /* ignore error from dict_number */

        if(res < 0)
        {
            reperr_module(create_error(ERR_SPELL), res);
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
*  delete word from dictionary
*
******************************************************************************/

extern void
DeleteWordFromDict_fn(void)
{
    S32 res;
    char array[LIN_BUFSIZ];

    if(!dialog_box_start())
        return;

    if( !insert_most_recent(&d_user_delete[1].textfield)                       ||
        err_open_master_dict()                                                 ||
        !mystr_set(&d_user_delete[0].textfield,
                   get_word_from_line(master_dictionary, array, lecpos, NULL))  )
        return;

    while(dialog_box(D_USER_DELETE))
    {
        res = dict_number(d_user_delete[1].textfield, TRUE);

        if(res >= 0)
            res = spell_deleteword(res, d_user_delete[0].textfield);

        if(res < 0)
        {
            reperr_module(create_error(ERR_SPELL), res);
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* flush user dictionary
*
******************************************************************************/

extern void
FlushUserDict_fn(void)
{
    flush_user_dictionaries();
}

/******************************************************************************
*
* insert word in dictionary
*
******************************************************************************/

extern void
InsertWordInDict_fn(void)
{
    STATUS res;
    char array[LIN_BUFSIZ];

    if(!dialog_box_start())
        return;

    if( !insert_most_recent(&d_user_insert[1].textfield)                       ||
        err_open_master_dict()                                                 ||
        !mystr_set(&d_user_insert[0].textfield,
                   get_word_from_line(master_dictionary, array, lecpos, NULL))  )
        return;

    while(dialog_box(D_USER_INSERT))
    {
        res = dict_number(d_user_insert[1].textfield, TRUE);

        if(res >= 0)
        {
            res = spell_addword(res, d_user_insert[0].textfield);

            if(res == 0)
                res = create_error(ERR_WORDEXISTS);
        }

        if(res < 0)
        {
            reperr_module(create_error(ERR_SPELL), res);
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/* all functions that use init,fill,end _box must have these icons */

#define browsing_Template   1
#define browsing_WordBox    2
#define browsing_ScrollUp   3    /* F2 */
#define browsing_ScrollDown 4    /* F3 */
#define browsing_PageUp     5    /* F4 */
#define browsing_PageDown   6    /* F5 */
#define browsing_FirstWord  7

/******************************************************************************
*
*  create/draw box to put browsed words in
*
******************************************************************************/

static BOOL
init_box(merge_dump_strukt *mdsp,
         const char *dname, const char *title, BOOL statik)
{
    S32 y;
    dbox d;
    void *core;
    STATUS status;
    char *errorp;

    trace_3(TRACE_APP_PD4, "init_box(" PTR_XTFMT ", %s, static = %s):", report_ptr_cast(mdsp), dname, trace_boolstring(statik));

    mdsp->res   = 0;
    mdsp->sofar = -1;
    mdsp->wants_nulls = FALSE;
    mdsp->stopped = FALSE;

    /* get some memory for the words array */

    if(NULL == (core = _al_ptr_alloc(sizeof(char [BROWSE_DEPTH][MAX_WORD+1]), &status)))
        return(reperr_null(status));

    mdsp->words = core;

    /* poke dbox with correct title */
    if(title)
        template_settitle(template_find_new(dname), title);

    /* create a dialog box */

    d = dbox_new_new(dname, &errorp);
    if(!d)
    {
        if(errorp)
            rep_fserr(errorp);
        al_ptr_free(core);
        return(FALSE);
    }

    mdsp->d = d;

    dbox_setfield(d, browsing_Template, NULLSTR);

    for(y = 0; y < BROWSE_DEPTH; ++y)
    {
        mdsp->words[y][0] = '\0';
        dbox_setfield(d, browsing_FirstWord + y, mdsp->words[y]);
    }

    if(statik)
        dbox_showstatic(d);
    else
        dbox_show(d);

    return(TRUE);
}

/******************************************************************************
*
* fill box with words
* first is a string to be put the top
* words is an array of [BROWSE_DEPTH][MAX_WORD+1] of uchar words
* lastlen is the corresponding latest lengths of the words
*
******************************************************************************/

static void
fill_box(
    char words[BROWSE_DEPTH][MAX_WORD+1],
    /*const*/ char *first,
         dbox d)
{
    S32 y;

    if(first)
        dbox_setfield(d, browsing_Template, first);

    for(y = 0; y < BROWSE_DEPTH; ++y)
        dbox_setfield(d, browsing_FirstWord + y, words[y]);
}

static void
end_box(
    merge_dump_strukt * mdsp)
{
    mdsp->dict = -1;

    al_ptr_dispose((P_P_ANY) /*_PEDANTIC*/ (&mdsp->words));

    dbox_dispose(&mdsp->d);
}

static void
scroll_words_up(
    char words[BROWSE_DEPTH][MAX_WORD+1],
    S32 depth)
{
    S32 y;

    for(y = 0; y < depth-1; ++y)
        strcpy(words[y], words[y+1]);
}

static void
scroll_words_down(
    char words[BROWSE_DEPTH][MAX_WORD+1],
    S32 depth)
{
    S32 y;

    for(y = depth-1; y >= 1; --y)
        strcpy(words[y], words[y-1]);
}

static char *browse_wild_str;
static char *browse_template;
static char *browse_wild_string;
static BOOL  browse_iswild;
static BOOL  browse_was_spell_err;

/* has writeable template changed? - checked on null events */

static void
browse_null_core(
    merge_dump_strukt * mdsp)
{
    dbox d = mdsp->d;
    char *str = (browse_iswild) ? browse_wild_str : browse_template;
    char array[MAX_WORD+1];

    trace_0(TRACE_APP_PD4, "browse_null()");

    dbox_getfield(d, browsing_Template, array, sizeof(array));

    if(0 != _stricmp(array, str))
    {
        if(browse_iswild)
        {
            bleep();
            dbox_setfield(d, browsing_Template, str);
        }
        else
        {
            trace_2(TRACE_APP_PD4, "template changed from %s to %s", str, array);
            if(array[0])
            {
                strcpy(str, array);
                mdsp->res = get_and_display_words(TRUE, browse_iswild, mdsp->words,
                                                  browse_wild_string, browse_template,
                                                  browse_wild_str, mdsp->dict,
                                                  d, &browse_was_spell_err);
                if(mdsp->res < 0)
                    win_send_close(dbox_syshandle(d));
                /* which will cause a CLOSE to be returned to fillin */
            }
            else
                trace_0(TRACE_APP_PD4, "string has been emptied");
        }
    }
    else
        trace_0(TRACE_APP_PD4, "no change in string");
}

extern void
browse_null(void)
{
    browse_null_core(&browse_mds);
}

#if 0
null_event_proto(static, browse_null_handler)
{
    switch(rc)
    {
    case NULL_QUERY:
        return(NULL_EVENTS_REQUIRED);

    case NULL_EVENT:
        browse_null_core(mdsp);

        return(NULL_EVENT_COMPLETED);

    default:
        return(0);
    }
}
#endif

/* get words from dictionary */

static S32
get_and_display_words(
    BOOL fillall,
    BOOL iswild,
    char words[BROWSE_DEPTH][MAX_WORD+1],
    char *wild_string,
    char *template,
    char *wild_str,
    _InVal_     DICT_NUMBER dict,
    dbox d,
    BOOL *was_spell_errp)
{
    S32 res = 1;
    PC_U8 src;
    P_U8 dst;
    S32 ch;
    S32 y;

    if(fillall)
    {
        #if 0
        strcpy(words[BROWSE_CENTRE], iswild ? NULLSTR : template);
        #else
        /* copy a correctly cased word to middle */
        src = iswild ? NULLSTR : template;
        dst = words[BROWSE_CENTRE];
        do  {
            ch = *src++;
            if(ch)
                ch = spell_tolower(dict, ch);
            *dst++ = ch;
        }
        while(ch);
        #endif

        trace_3(TRACE_APP_PD4, "fillall: iswild = %s, wild = '%s', template = '%s'",
                trace_boolstring(iswild), wild_string, template);

        if(iswild  ||  ((res = spell_checkword(dict, template)) == 0))
            if((res = spell_nextword(dict, words[BROWSE_CENTRE], words[BROWSE_CENTRE], wild_string, &ctrlflag)) == 0)
                res = spell_prevword(dict, words[BROWSE_CENTRE], template, wild_string, &ctrlflag);

        for(y = BROWSE_CENTRE + 1;
            !ctrlflag  &&  (res >= 0)  &&  (y < BROWSE_DEPTH);
            ++y)
            if(!*words[y-1]  ||  ((res = spell_nextword(dict, words[y], words[y-1], wild_string, &ctrlflag)) <= 0))
                words[y][0] = '\0';
            else if(iswild)
                fill_box(words, wild_str, d);

        for(y = BROWSE_CENTRE-1; !ctrlflag  &&  (y >= 0) &&  (res >= 0); --y)
            if(!*words[y+1]  ||  ((res = spell_prevword(dict, words[y], words[y+1], wild_string, &ctrlflag)) <= 0))
                words[y][0] = '\0';

        if(ctrlflag)
        {
            ack_esc();
            return(create_error(ERR_ESCAPE));
        }

        if(res < 0)
        {
            *was_spell_errp = TRUE;
            return(res);
        }
    }

    fill_box(words, iswild ? wild_str : template, d);

    return(res);
}

/******************************************************************************
*
* catch raw events sent to the browse dbox and
* turn (Page)Up/Down events into button hits
*
******************************************************************************/

static BOOL
browse_raw_eventhandler(
    dbox d,
    void *event,
    void *handle)
{
    wimp_eventstr *e = (wimp_eventstr *) event;
    S32 hiticon;

    IGNOREPARM(d);
    IGNOREPARM(handle);

    trace_1(TRACE_APP_PD4, "raw_event_browse got %s", report_wimp_event(e->e, &e->data));

    switch(e->e)
    {
    case wimp_EKEY:
        switch(e->data.key.chcode)
        {
        case akbd_UpK:
        case akbd_DownK:
            hiticon = (e->data.key.chcode == akbd_UpK)
                                ? browsing_ScrollUp
                                : browsing_ScrollDown;
            break;

        case akbd_PageUpK:
        case akbd_PageDownK:
            hiticon = (e->data.key.chcode == akbd_PageUpK)
                                ? browsing_PageUp
                                : browsing_PageDown;
            break;

        default:
            hiticon = -1;
            break;
        }

        if(hiticon != -1)
        {
            e->e                = wimp_EBUT;
            e->data.but.m.i     = (wimp_i) hiticon;
            e->data.but.m.bbits = wimp_BLEFT;

            clearkeyboardbuffer();
        }
        break;

    default:
        break;
    }

    return(FALSE);
}

static void
browse_process(void)
{
    merge_dump_strukt *mdsp = &browse_mds;
    dbox d = mdsp->d;
    dbox_field f;
    BOOL adjustclicked;
    const DICT_NUMBER dict = mdsp->dict;
    char (*words)[MAX_WORD+1] = mdsp->words;
    S32 i;
    S32 which = -1;        /* which word was clicked on */
    BOOL fillall;
    char array[MAX_WORD+1];

    if(!*browse_template)
    {
        /* start off at 'a' if nothing specified */
        *browse_template   = 'a';
        browse_template[1] = '\0';
    }

    escape_enable();

    mdsp->res = get_and_display_words(TRUE, browse_iswild, words,
                                      browse_wild_string, browse_template,
                                      browse_wild_str, dict,
                                      d,
                                      &browse_was_spell_err);

    if(mdsp->res < 0)
    {
        if(browse_was_spell_err)
            reperr_module(create_error(ERR_SPELL), mdsp->res);
        else
            reperr_null(mdsp->res);

        (void) escape_disable_nowinge();
        return;
    }

    if(escape_disable())
        return;

    browsing_docno = current_docno();

    dbox_raw_eventhandler(d, browse_raw_eventhandler, NULL);

#if 0
    status_assert(Null_EventHandler(browse_null_handler, mdsp, TRUE, 0));
#endif

    while(((f = riscdialog_fillin_for_browse(d)) != dbox_CLOSE)  &&
          (f != dbox_OK))
    {
        adjustclicked = riscos_adjustclicked();

        (void) dbox_adjusthit(&f, browsing_ScrollUp, browsing_ScrollDown, adjustclicked);
        (void) dbox_adjusthit(&f, browsing_PageUp,   browsing_PageDown,   adjustclicked);

        fillall = FALSE;

        escape_enable();

        switch(f)
        {
        case browsing_ScrollUp:
            if(*words[BROWSE_CENTRE-1])
            {
                scroll_words_down(words, BROWSE_DEPTH);

                if(!browse_iswild)
                    strcpy(browse_template, words[BROWSE_CENTRE]);

                if(*words[0])
                    mdsp->res = spell_prevword(dict,
                                               words[0],
                                               words[0],
                                               browse_wild_string,
                                               &ctrlflag);
            }
            break;

        case browsing_ScrollDown:
            if(*words[BROWSE_CENTRE+1])
            {
                scroll_words_up(words, BROWSE_DEPTH);

                if(!browse_iswild)
                    strcpy(browse_template, words[BROWSE_CENTRE]);

                if(*mdsp->words[BROWSE_DEPTH-1])
                    mdsp->res = spell_nextword(dict,
                                               words[BROWSE_DEPTH-1],
                                               words[BROWSE_DEPTH-1],
                                               browse_wild_string,
                                               &ctrlflag);
            }
            break;

        case browsing_PageUp:
            if(browse_iswild)
                bleep();
            else
            {
                mdsp->res = 1;

                for(i = 0; (mdsp->res > 0)  &&  (i < BROWSE_DEPTH); ++i)
                {
                    strcpy(array, browse_template);

                    mdsp->res = spell_prevword(dict,
                                               browse_template,
                                               browse_template,
                                               browse_wild_string,
                                               &ctrlflag);

                    if(!mdsp->res)
                    {
                        /* restore template if no word found */
                        strcpy(browse_template, array);
                        break;
                    }
                }

                fillall = TRUE;
            }
            break;

        case browsing_PageDown:
            if(browse_iswild)
                bleep();
            else
            {
                mdsp->res = 1;

                for(i = 0; (mdsp->res > 0)  &&  (i < BROWSE_DEPTH); ++i)
                {
                    strcpy(array, browse_template);

                    mdsp->res = spell_nextword(dict,
                                               browse_template,
                                               browse_template,
                                               browse_wild_string,
                                               &ctrlflag);

                    if(!mdsp->res)
                    {
                        /* restore template if no word found */
                        strcpy(browse_template, array);
                        break;
                    }
                }

                fillall = TRUE;
            }
            break;

        default:
            /* clicks on words enter them */
            which = f - browsing_FirstWord;
            if( (which >= 0)  &&  (which < BROWSE_DEPTH)  &&
                *words[which])
                    goto LOOP_EXIT;
            else
                which = -1;
            break;
        }

        mdsp->res = get_and_display_words(fillall, browse_iswild, words,
                                          browse_wild_string, browse_template,
                                          browse_wild_str, dict,
                                          d,
                                          &browse_was_spell_err);

        if(mdsp->res < 0)
        {
            escape_disable();
            break;
        }

    LOOP_EXIT:

        if(escape_disable_nowinge())
        {
            mdsp->res = create_error(ERR_ESCAPE);
            break;
        }

        if(which >= 0)
        {
            f = dbox_OK;
            break;
        }
    }

    /* click/type-ahead may have resulted in string emptying
     * without us getting to see it
    */
    if((mdsp->res >= 0)  &&  (f == dbox_OK))
    {
        if(which >= 0)
            strcpy(word_to_insert, words[which]);
        else
        {
            dbox_getfield(d, browsing_Template, browse_template, LIN_BUFSIZ);

            if(!browse_template[0])
            {
                word_to_insert[0] = 'a';
                word_to_insert[1] = browse_template[0];
            }
            else
            {
                if(!browse_iswild)
                    strcpy(word_to_insert, browse_template);
                else
                    strcpy(word_to_insert, words[BROWSE_CENTRE]);
            }
        }
    }

    trace_1(TRACE_APP_PD4, "returning word_to_insert \"%s\"", word_to_insert);

#if 0
    status_assert(Null_EventHandler(browse_null_handler, mdsp, FALSE, 0));
#endif

    browsing_docno = DOCNO_NONE;
}

/******************************************************************************
*
*  browse through a dictionary
*
******************************************************************************/

static S32
browse(
    _InVal_     DICT_NUMBER dict,
    char *wild_str)
{
    char template[LIN_BUFSIZ];
    char wild_string[LIN_BUFSIZ];
    merge_dump_strukt *mdsp = &browse_mds;

    /* compile the wild string */
    *template = '\0';

    if(!wild_str)
        wild_str = NULLSTR;

    browse_iswild = compile_wild_string(dict, wild_string, wild_str);
    if(browse_iswild  < 0)
        return(browse_iswild);

    if(!browse_iswild)
    {
        *wild_string = '\0';
        strcpy(template, wild_str);
    }

    *word_to_insert = template[MAX_WORD] = '\0';

    mdsp->dict = dict;

    browse_wild_str        = wild_str;
    browse_template        = template;
    browse_wild_string    = wild_string;

    if(init_box(mdsp, "browsing", NULL, FALSE))
    {
        browse_process();

        end_box(mdsp);
    }

    return(mdsp->res);
}

/******************************************************************************
*
*  open dictionary and browse through it reporting errors
*
******************************************************************************/

extern void
BrowseDictionary_fn(void)
{
    S32 res;
    char array[LIN_BUFSIZ];
    S32 found_offset;

    if(!dialog_box_start())
        return;

    d_user_browse[1].option = 'N';

    if( !insert_most_recent(&d_user_browse[1].textfield)                                ||
        err_open_master_dict()                                                          ||
        !mystr_set(&d_user_browse[0].textfield,
                   get_word_from_line(master_dictionary, array, lecpos, &found_offset))  )
        return;

    if(!dialog_box(D_USER_BROWSE))
        return;

    dialog_box_end();

    browse_was_spell_err = FALSE;

    res = open_appropriate_dict(&d_user_browse[1]);

    if(res >= 0)
        res = browse(res, d_user_browse[0].textfield);

    if(res < 0)
    {
        if(browse_was_spell_err)
            reperr_module(create_error(ERR_SPELL), res);
        else
            reperr_null(res);
    }
    else if(*word_to_insert && is_current_document())
    {
        trace_1(TRACE_APP_PD4, "SBRfunc * word_to_insert: %s", trace_string(word_to_insert));

        /* save word about to be trashed onto paste list */
        save_words(array);

        sch_pos_end.col = sch_pos_stt.col = curcol;
        sch_pos_end.row = sch_pos_stt.row = currow;
        sch_stt_offset  = found_offset;
        sch_end_offset  = sch_stt_offset + strlen(array);

        if(!do_replace(word_to_insert, TRUE))
            bleep();
    }
}

/*
 * get all anagrams of string
 * return FALSE if error or escape
 * TRUE otherwise
*/

/*
 * copy the matching letters from lastword into array
 * fill out array with smallest word
*/

static BOOL
a_fillout(
    A_LETTER *letters,
    char *array,
    char *lastword,
    BOOL subgrams)
{
    A_LETTER *aptr = letters;
    uchar *from = lastword;
    uchar *to = array;
    S32 length = 0;
    BOOL dontfillout = FALSE;

    /* set all letters unused */
    while(aptr->letter)
    {
        (aptr++)->used = FALSE;
        length++;
    }

    /* get the common letters */
    for( ; *from; from++)
        for(aptr = letters; ; aptr++)
        {
            if(!aptr->letter)
            {
                /* need to step back letter */
                dontfillout = TRUE;

                for(;;)
                {
                    uchar letter_to_free;
                    A_LETTER *smallest_bigger = NULL;

                    /* maybe no letters to step back */
                    if(--to < array)
                        return(FALSE);

                    /* free the letter that is here and look for new letter */
                    letter_to_free = *to;

                    /* search backwards, picking up the smallest bigger letter */
                    for(aptr = letters + length; ; --aptr)
                    {
                        if(aptr->used == FALSE)
                        {
                            if(aptr->letter > letter_to_free)
                            {
                                smallest_bigger = aptr;
                                continue;
                            }
                        }
                        else if(aptr->letter == letter_to_free)
                        {
                            aptr->used = FALSE;

                            if(smallest_bigger != NULL)
                            {
                                smallest_bigger->used = TRUE;
                                *to++ = smallest_bigger->letter;
                                goto DO_FILL;
                            }
                            /* go back another letter */
                            break;
                        }
                    }
                }
            }

            if(aptr->used)
                continue;

            if(*from < aptr->letter)
            {
                /* no match so take next biggest letter */
                aptr->used = TRUE;
                *to++ = aptr->letter;
                dontfillout = TRUE;
                goto DO_FILL;
            }

            if(*from == aptr->letter)
            {
                /* this letter matches, flag it and go round again */
                aptr->used = TRUE;
                *to++ = *from;
                break;
            }
        }

DO_FILL:

    /* now fill out with remains */
    if(!subgrams  ||  (subgrams  &&  !dontfillout  &&  (to == array)))
        for(aptr = letters; aptr->letter; aptr++)
            if(aptr->used == FALSE)
            {
                *to++ = aptr->letter;
                if(subgrams)
                    break;
            }

    *to = '\0';
    return(TRUE);
}

static void
add_word_to_box(
    merge_dump_strukt *mdsp,
    const char *str)
{
    trace_1(TRACE_APP_PD4, "adding word %s to box", str);

    if(++(mdsp->sofar) >= BROWSE_DEPTH)
    {
        scroll_words_up(mdsp->words, BROWSE_DEPTH);
        mdsp->sofar = BROWSE_DEPTH-1;
    }

    strcpy((mdsp->words)[mdsp->sofar], str);

    fill_box(mdsp->words, NULL, mdsp->d);
}

static void
add_error_to_box(
    merge_dump_strukt * mdsp)
{
    char buffer[256];

    bleep();

    add_word_to_box(mdsp, NULLSTR);
    xstrkpy(buffer, elemof32(buffer), "*** ");
    xstrkat(buffer, elemof32(buffer), reperr_getstr(mdsp->res));
    xstrkat(buffer, elemof32(buffer), " ***");
    add_word_to_box(mdsp, buffer);

    /* error now reported, clear down and pause */
    mdsp->res = 0;
    mdsp->wants_nulls = FALSE;
}

static void
complete_box(
    merge_dump_strukt * mdsp,
    const char * str)
{
    char array[64];

    (void) xsnprintf(array, elemof32(array), Zs_complete_STR, str);

    add_word_to_box(mdsp, NULLSTR);
    add_word_to_box(mdsp, array);
}

static char     anagram_last_found[MAX_WORD+1];
static BOOL     anagram_sub;
static A_LETTER anagram_letters[MAX_WORD+1];

null_event_proto(static, anagram_null_handler);

/* mergedict complete - tidy up */

static void
anagram_end(
    merge_dump_strukt * mdsp)
{
    mdsp->wants_nulls = FALSE;

    Null_EventHandlerRemove(anagram_null_handler, mdsp);

    if(mdsp->res < 0)
        reperr_module(create_error(ERR_SPELL), mdsp->res);

    end_box(mdsp);
}

#define anagram_Stop     0
#define anagram_Pause    3
#define anagram_Continue 4

/* anagram upcall processor */

static void
anagram_eventhandler(
    dbox d,
    void * handle)
{
    merge_dump_strukt * mdsp = handle;
    dbox_field f = dbox_get(d);

    switch(f)
    {
    case anagram_Continue:
        /* only resume if paused and not ended */
        if(!mdsp->stopped)
            mdsp->wants_nulls = TRUE;
        break;

    case anagram_Pause:
        /* stop null events to anagram null processor */
        mdsp->wants_nulls = FALSE;
        break;

    default:
        anagram_end(mdsp);
        break;
        }
}

extern void
anagram_null_core(
    merge_dump_strukt * mdsp);
extern void
anagram_null_core(
    merge_dump_strukt * mdsp)
{
    char newword[MAX_WORD+1];
    S32 res1;

    trace_0(TRACE_APP_PD4, "anagram_null()");

    if(a_fillout(anagram_letters, newword, anagram_last_found, anagram_sub))
    {
        /* SKS - stop us from getting bad words */
        if(spell_valid_1(mdsp->dict, *newword))
        {
            mdsp->res = spell_checkword(mdsp->dict, newword);

            if(mdsp->res < 0)
            {
                add_error_to_box(mdsp);
                return;
            }

            /* don't like most single letter words */
            if( mdsp->res  &&
                ((newword[1])  || (*newword == 'i') ||  (*newword == 'a'))
                )
                add_word_to_box(mdsp, newword);
        }

        escape_enable();

        mdsp->res = spell_nextword(mdsp->dict, anagram_last_found, newword, NULL, &ctrlflag);

        res1 = escape_disable_nowinge();

        if((res1 < 0) && (mdsp->res >= 0))
            mdsp->res = res1;

        if(mdsp->res < 0)
        {
            add_error_to_box(mdsp);
            return;
        }
    }
    else
    {
        mdsp->res = 0;
        mdsp->stopped = TRUE;
    }

    if(!mdsp->res)
    {
        complete_box(mdsp, anagram_sub ? Subgrams_STR : Anagrams_STR);

        /* force punter to do explicit end */
        mdsp->wants_nulls = FALSE;
        return;
    }
}

null_event_proto(static, anagram_null_handler)
{
    merge_dump_strukt * mdsp = (merge_dump_strukt *) p_null_event_block->client_handle;

    switch(p_null_event_block->rc)
    {
    case NULL_QUERY:
        return(mdsp->wants_nulls
                        ? NULL_EVENTS_REQUIRED
                        : NULL_EVENTS_OFF);

    case NULL_EVENT:
        anagram_null_core(mdsp);

        return(NULL_EVENT_COMPLETED);

    default:
        return(NULL_EVENT_UNKNOWN);
    }
}

static void
anagram_start(
    merge_dump_strukt * mdsp)
{
    mdsp->wants_nulls = TRUE;

    status_assert(Null_EventHandlerAdd(anagram_null_handler, mdsp, 0));

    dbox_eventhandler(mdsp->d, anagram_eventhandler, mdsp);

    /* all anagram searching done on null events, button processing on upcalls */
}

/*
 * find anagrams in the specified dictionary of this collection of letters
 * assumes letters sorted into order
 * works like pair of co-routines
 *        a_fillout gets asked to supply next possible string
 *        spell_next gets asked for next word in dictionary
*/

/*
top-level routine for anagrams and subgrams
*/

static void
ana_or_sub_gram(
    BOOL sub)
{
    merge_dump_strukt * mdsp = &anagram_mds;
    S32 res;
    char *word;
    char array[MAX_WORD+1];
    A_LETTER * aptr;
    uchar * from, * to;
    char ch;

    if(mdsp->d)
    {
        reperr_null(anagram_sub ? create_error(ERR_ALREADYSUBGRAMS) : create_error(ERR_ALREADYANAGRAMS));
        return;
    }

    if(!dialog_box_start())
        return;

    if(!insert_most_recent(&d_user_anag[1].textfield))
        return;

    while(dialog_box(sub ? D_USER_SUBGRAM : D_USER_ANAG))
    {
        word = d_user_anag[0].textfield;

        if(str_isblank(word)  ||  (strlen(word) > MAX_WORD))
        {
            reperr_null(create_error(SPELL_ERR_BADWORD));
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        dialog_box_end();

        mdsp->dict = open_appropriate_dict(&d_user_anag[1]);

        res = (mdsp->dict < 0)
                ? mdsp->dict
                : compile_wild_string(mdsp->dict, array, word);

        if(res < 0)
        {
            reperr_null(res);
            break;
        }

        /* sort letters into order */
        from = word;
        *array = '\0';
        while((ch = *from++) != '\0')
        {
            ch = spell_tolower(mdsp->dict, ch);

            for(to = array; ; to++)
                if(!*to  ||  (*to > ch))
                {
                    memmove32(to + 1, to, strlen32p1((char *) to));
                    *to = ch;
                    break;
                }
        }

        trace_1(TRACE_APP_PD4, "array=_%s_", array);

        /* copy letters into letters array */
        from = array;
        aptr = anagram_letters;
        do  {
            ch = *from++;
            aptr++->letter = ch;
        }
        while(ch);

        *anagram_last_found = '\0';

        anagram_sub = sub;

        if(init_box(mdsp, "anagram", sub ? Subgrams_STR : Anagrams_STR, TRUE))
        {
            dbox_setfield(mdsp->d, browsing_Template, d_user_anag[0].textfield);

            anagram_start(mdsp);
            return;
        }

        anagram_end(mdsp);

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* anagrams
*
******************************************************************************/

extern void
Anagrams_fn(void)
{
    ana_or_sub_gram(FALSE);
}

/******************************************************************************
*
* subgrams
*
******************************************************************************/

extern void
Subgrams_fn(void)
{
    ana_or_sub_gram(TRUE);
}

/******************************************************************************
*
*  switch auto checking on or off
*
******************************************************************************/

extern void
AutoSpellCheck_fn(void)
{
    d_auto[0].option = (optiontype) (d_auto[0].option ^ ('Y' ^ 'N'));

    check_state_changed();
}

/******************************************************************************
*
*  check word is not in the user dictionaries
*
******************************************************************************/

static BOOL
not_in_user_dicts_or_list(
    const char *word)
{
    PC_LIST lptr;

    for(lptr = first_in_list(&first_spell);
        lptr;
        lptr = next_in_list(&first_spell))
    {
        if(0 == _stricmp((char *) lptr->value, word))
            return(FALSE);
    }

    for(lptr = first_in_list(&first_user_dict);
        lptr;
        lptr = next_in_list(&first_user_dict))
    {
        S32 res;

        if((res = spell_checkword((S32) lptr->key, (char *) word)) > 0)
            return(FALSE);

        if((res < 0)  &&  (res != create_error(SPELL_ERR_BADWORD)))
            reperr_module(create_error(ERR_SPELL), res);
    }

    return(TRUE);
}

#define C_BLOCK 0
#define C_ALL 1

/*
 * set up the block to check
*/

static BOOL
set_check_block(void)
{
    /* set up the check block */

    if(d_checkon[C_BLOCK].option == 'Y')
    {
        /* marked block selected - check exists */
        if(!MARKER_DEFINED())
            return(reperr_null((blkstart.col != NO_COL)
                                        ? create_error(ERR_NOBLOCKINDOC)
                                        : create_error(ERR_NOBLOCK)));

        sch_stt = blkstart;
        sch_end = (blkend.col != NO_COL) ? blkend : blkstart;
    }
    else
    {
        /* all rows, maybe all files */

        sch_stt.row = 0;
        sch_end.row = numrow-1;

        sch_stt.col = 0;
        sch_end.col = numcol-1;
    }

    /* set up the initial position */

    sch_pos_stt.col = sch_stt.col - 1;
    sch_pos_stt.row = sch_stt.row;
    sch_stt_offset  = -1;

    return(TRUE);
}

/*
 * readjust sch_stt_offset at start of next word on line
 * assumes line in buffer
*/

static BOOL
next_word_on_line(void)
{
    S32 len;
    PC_U8 ptr;
    DICT_NUMBER dict;

    len = strlen(linbuf);

    if(sch_stt_offset >= len)
        return(FALSE);

    /* if pointing to a word, skip it */
    ptr  = linbuf + sch_stt_offset;
    dict = master_dictionary;

    if(browse_valid_1(dict, *ptr++))
        do
            ;
        while(browse_iswordc(dict, *ptr++));

    sch_stt_offset = (--ptr) - linbuf;

    while(!browse_valid_1(dict, *ptr)  &&  (sch_stt_offset < len))
    {
        ++ptr;
        ++sch_stt_offset;
    }

    return(sch_stt_offset < len);
}

/*
 * find the next mis-spelt word starting with the current word
*/

static BOOL
get_next_misspell(
    char *array /*out*/)
{
    S32 res, dict;
    P_CELL tcell;
    BOOL tried_again;

    trace_1(TRACE_APP_PD4, "get_next_misspell(): sch_stt_offset = %d\n]", sch_stt_offset);

    dict = master_dictionary;

    for(;;)
    {
        if((sch_stt_offset == -1)  ||  !slot_in_buffer)
        {
            if(sch_stt_offset == -1)
            {
                /* (NB. set sch_pos_stt.col = sch_stt.col - 1 to start) */

                trace_0(TRACE_APP_PD4, "get_next_misspell: offset == -1 -> find another cell to scan for misspells");

                do  {
                    if(sch_pos_stt.col < sch_end.col)
                    {
                        ++sch_pos_stt.col;
                        trace_1(TRACE_APP_PD4, "get_next_misspell stepped to column %d", sch_pos_stt.col);
                    }
                    else if(sch_pos_stt.row < sch_end.row)
                    {
                        ++sch_pos_stt.row;
                        sch_pos_stt.col = sch_stt.col;
                        trace_2(TRACE_APP_PD4, "get_next_misspell stepped to row %d, reset to column %d", sch_pos_stt.row, sch_pos_stt.col);
                        actind((S32) ((100 * sch_pos_stt.row - sch_stt.row) / (sch_end.row - sch_stt.row + 1)));
                        if(ctrlflag)
                            return(FALSE);
                    }
                    else
                    {
                        trace_0(TRACE_APP_PD4, "get_next_misspell ran out of cells");
                        return(FALSE);
                    }

                    tcell = travel(sch_pos_stt.col, sch_pos_stt.row);
                }
                while(!tcell  ||  (tcell->type != SL_TEXT)  ||  str_isblank(tcell->content.text));

                sch_stt_offset = 0;
            }
            else
            {
                trace_0(TRACE_APP_PD4, "get_next_misspell found no cell in buffer so reload");
                tcell = travel(sch_pos_stt.col, sch_pos_stt.row);
            }

            prccon(linbuf, tcell);
        }

        /* if current word not ok set variables */

        if(browse_valid_1(dict, linbuf[sch_stt_offset]))
        {
            tried_again = FALSE;

            slot_in_buffer = TRUE;
            (void) get_word_from_line(dict, array, sch_stt_offset, NULL);
            slot_in_buffer = FALSE;

        TRY_AGAIN:

            res = spell_checkword(dict, array);

            if(res == create_error(SPELL_ERR_BADWORD))
                res = 0;

            if(res < 0)
                return(reperr_module(create_error(ERR_SPELL), res));

            if(!res  &&  (not_in_user_dicts_or_list(array)))
            {
                /* can't find it anywhere: try stripping off trailing ' or 's */

                if(!tried_again)
                    if(worth_trying_again(dict, array))
                    {
                        tried_again = TRUE;
                        goto TRY_AGAIN;
                    }

                /* not in any dictionary, with or without quote: move there! */
                /* chknlr() sets the state to be picked up by a draw_screen() */
                trace_2(TRACE_APP_PD4, "get_next_misspell moving to %d, %d", sch_pos_stt.col, sch_pos_stt.row);
                chknlr(sch_pos_stt.col, sch_pos_stt.row);
                return(TRUE);
                }
        }

        /* go to next word if available, else reload from another cell */
        if(!next_word_on_line())
            sch_stt_offset = -1;
    }
}

#define C_CHANGE 0
#define C_BROWSE 1
#define C_ADD    2

/******************************************************************************
*
*  check current file
*
******************************************************************************/

extern void
CheckDocument_fn(void)
{
    SLR oldpos;
    S32 tlecpos;
    S32 mis_spells = 0, words_added = 0, res;
    char array[MAX_WORD];
    char original[LIN_BUFSIZ];
    char array1[MAX_WORD+1];
    char array2[MAX_WORD+1];
    BOOL do_disable = FALSE;

    if(!init_dialog_box(D_CHECKON))
        return;

    if(!dialog_box_start())
        return;

    if(!dialog_box(D_CHECKON))
        return;

    dialog_box_end();

    if( !mergebuf_nocheck()    ||
        !set_check_block()     ||
        err_open_master_dict() )
            return;

    /* must be at least one line we can get to */
    if(n_rowfixes >= rowsonscreen)
        internal_process_command(N_FixRows);

    /* save current position */
    oldpos.col = curcol;
    oldpos.row = currow;
    tlecpos    = lecpos;

    escape_enable(); do_disable = TRUE;

    while(!been_error)
    {
        /* find next mis-spelled word */
        for(; get_next_misspell(array) && !been_error && !ctrlflag; ++mis_spells)
        {
            actind_end();

            escape_disable(); do_disable = FALSE;

            /* NB. lowercase version! */
            if(!mystr_set(&d_check[C_CHANGE].textfield, array))
                break;

            d_check[C_CHANGE].option = d_check[C_BROWSE].option = 'N';

            /* take a copy of just the misplet worm */
            xstrnkpy(original, elemof32(original), linbuf + sch_stt_offset, strlen(array));
            trace_1(TRACE_APP_PD4, "CheckDocument_fn: misplet word is '%s'", original);

            word_to_invert = original;
            lecpos = sch_stt_offset;

            /* desperate attempts to get correct redraw */
            output_buffer = TRUE;
            slot_in_buffer = TRUE;
            draw_screen();
            draw_caret(); /* otherwise caret will keep flashing on/off at wrong place */
            slot_in_buffer = FALSE;

        DOG_BOX:
            (void) insert_most_recent(&d_check[C_ADD].textfield);

            clearkeyboardbuffer();

            if(!dialog_box_start()  ||  !dialog_box(D_CHECK))
            {
                been_error = TRUE;
                break;
            }

            dialog_box_end();

            /* add to user dictionary? */
            if(d_check[C_ADD].option == 'Y')
            {
                res = dict_number(d_check[C_ADD].textfield, TRUE);

                if(status_ok(res))
                {
                    if((res = spell_addword((DICT_NUMBER) res,
                                            d_check[C_CHANGE].textfield))
                                            > 0)
                        ++words_added;
                }

                if(res < 0)
                {
                    reperr_module(create_error(ERR_SPELL), res);
                    break;
                }
            }

            /* browse? */
            if(d_check[C_BROWSE].option == 'Y')
            {
                res = browse(master_dictionary, d_check[C_CHANGE].textfield);

                trace_1(TRACE_APP_PD4, "word_to_insert = _%s_", word_to_insert);

                if(res < 0)
                {
                    reperr_module(create_error(ERR_SPELL), res);
                    break;
                }
                else if(*word_to_insert)
                {
                    mystr_set(&d_check[C_CHANGE].textfield, word_to_insert);
                    d_check[C_CHANGE].option = 'Y';
                }

                d_check[C_BROWSE].option = 'N';
                goto DOG_BOX;
            }

            /* redraw the row sometime */
            word_to_invert = NULL;
            mark_row(currowoffset);

            /* replace word? */
            if(d_check[C_CHANGE].option == 'Y')
            {
                sch_pos_end.col = sch_pos_stt.col;
                sch_pos_end.row = sch_pos_stt.row;
                sch_end_offset  = sch_stt_offset + strlen((char *) array);

                if(protected_cell(sch_pos_stt.col, sch_pos_stt.row))
                    break;

                filbuf();

                res = do_replace((uchar *) d_check[C_CHANGE].textfield, TRUE);

                if(!res)
                {
                    bleep();
                    break;
                }

                if(!mergebuf_nocheck())
                    break;
            }
            /* if all set to no, add to temporary spell list */
            else if((d_check[C_BROWSE].option == 'N')  &&  (d_check[C_ADD].option == 'N'))
            {
                if(status_fail(res = add_to_list(&first_spell, 0, array)))
                {
                    reperr_null(res);
                    break;
                }
            }

            escape_enable(); do_disable = TRUE;
        } /*for()*/

        break;
    }

    if(do_disable)
        escape_disable();

    actind_end();

    word_to_invert = NULL;

    if(is_current_document())
    {
        /* restore old position - don't do mergebuf */
        slot_in_buffer = buffer_altered = FALSE;

            {
            chknlr(oldpos.col, oldpos.row);
            lecpos = tlecpos;
            }

        out_screen = xf_drawcolumnheadings = xf_interrupted = TRUE;

        draw_screen();
        draw_caret();
    }

    if(!in_execfile)
    {
        /* put up message box saying what happened */
        (void) xsnprintf(array1, elemof32(array1),
                (mis_spells == 1)
                        ? Zd_unrecognised_word_STR
                        : Zd_unrecognised_words_STR,
                mis_spells);
        (void) xsnprintf(array2, elemof32(array2),
                (words_added == 1)
                        ? Zd_word_added_to_user_dict_STR
                        : Zd_words_added_to_user_dict_STR,
                words_added);

        d_check_mess[0].textfield = array1;
        d_check_mess[1].textfield = array2;

        if(dialog_box_start())
        {
            (void) dialog_box(D_CHECK_MESS);

            dialog_box_end();
        }
    }
}

/******************************************************************************
*
*                        Merge file with dictionary
*
******************************************************************************/

/******************************************************************************
*
*  get the next word from the file
*
******************************************************************************/

static BOOL
get_word_from_file(
    _InVal_     DICT_NUMBER dict,
    FILE_HANDLE in,
    char *array /*out*/)
{
    S32 c;
    char *ptr;

    for(;;)
    {
        ptr = array;

        for(;;)
        {
            c = pd_file_getc(in);

            trace_1(TRACE_APP_PD4, "get_word_from_file: looking for good first char - trying %2.2X", c);

            if(c == EOF)
                return(FALSE);

            if(spell_valid_1(dict, c))
                break;
        }

        trace_2(TRACE_APP_PD4, "get_word_from_file: got good first char %2.2X, %c", c, c);

        /* c is first letter of word. Ignore overlong words */

        for(;;)
        {
            if(ptr - array <= MAX_WORD)
                *ptr++ = (char) c;

            c = pd_file_getc(in);

            trace_1(TRACE_APP_PD4, "get_word_from_file: looking for more valid chars - trying %2.2X", c);

            if((c == EOF)  ||  !spell_iswordc(dict, c))
            {
                if(ptr - array > MAX_WORD)
                {
                    if(c == EOF)
                        return(FALSE);
                    else
                        break;
                }

                *ptr = '\0';

                trace_1(TRACE_APP_PD4, "get_word_from_file: got word '%s'", array);
                return(TRUE);
            }
        }
    }
}

static FILE_HANDLE mergedict_in   = NULL;
static char   mergedict_array[MAX_WORD+1];

null_event_proto(static, mergedict_null_handler);

static void
mergedict_close(
    merge_dump_strukt * mdsp)
{
    mdsp->stopped = TRUE;

    pd_file_close(&mergedict_in);
}

/* mergedict complete - tidy up */

static void
mergedict_end(
    merge_dump_strukt * mdsp)
{
    mdsp->wants_nulls = FALSE;

    Null_EventHandlerRemove(mergedict_null_handler, mdsp);

    if(mdsp->res < 0)
        reperr_module(create_error(ERR_SPELL), mdsp->res);

    mergedict_close(mdsp);

    end_box(mdsp);
}

#define mergedict_Stop     0
#define mergedict_Pause    3
#define mergedict_Continue 4

/* mergedict upcall processor */

static void
mergedict_eventhandler(
    dbox d,
    void * handle)
{
    merge_dump_strukt * mdsp = handle;
    dbox_field f = dbox_get(d);

    trace_2(TRACE_APP_PD4, "mergedict_eventhandler got button %d: mergedict_in = " PTR_XTFMT, f, report_ptr_cast(mergedict_in));

    switch(f)
    {
    case mergedict_Continue:
        /* only resume if paused and not ended */
        if(!mdsp->stopped)
            mdsp->wants_nulls = TRUE;
        break;

    case mergedict_Pause:
        /* stop null events to mergedict null processor */
        mdsp->wants_nulls = FALSE;
        break;

    default:
        mergedict_end(mdsp);
        break;
        }
}

/* mergedict null processor */

static void
mergedict_null_core(
    merge_dump_strukt * mdsp)
{
    trace_0(TRACE_APP_PD4, "mergedict_null()");

    if(!get_word_from_file(mdsp->dict, mergedict_in, mergedict_array))
    {
        complete_box(mdsp, Merge_STR);

        mergedict_close(mdsp);

        /* force punter to do explicit end */
        mdsp->wants_nulls = FALSE;
        return;
    }

    mdsp->res = spell_addword(mdsp->dict, mergedict_array);

    if(mdsp->res != 0)
    {
        add_word_to_box(mdsp, mergedict_array);

        if(mdsp->res < 0)
            add_error_to_box(mdsp);
    }
}

null_event_proto(static, mergedict_null_handler)
{
    merge_dump_strukt * mdsp = (merge_dump_strukt *) p_null_event_block->client_handle;

    switch(p_null_event_block->rc)
    {
    case NULL_QUERY:
        return(mdsp->wants_nulls
                        ? NULL_EVENTS_REQUIRED
                        : NULL_EVENTS_OFF);

    case NULL_EVENT:
        mergedict_null_core(mdsp);

        return(NULL_EVENT_COMPLETED);

    default:
        return(NULL_EVENT_UNKNOWN);
    }
}

static void
mergedict_start(
    merge_dump_strukt * mdsp)
{
    mdsp->wants_nulls = TRUE;

    status_assert(Null_EventHandlerAdd(mergedict_null_handler, mdsp, 0));

    dbox_eventhandler(mdsp->d, mergedict_eventhandler, mdsp);

    /* all merging done on null events, button processing on upcalls */
}

/******************************************************************************
*
*  merge file with user dictionary
*
******************************************************************************/

extern void
MergeFileWithDict_fn(void)
{
    char buffer[BUF_MAX_PATHSTRING];
    char *name;
    merge_dump_strukt *mdsp = &mergedict_mds;

    if(mdsp->d)
    {
        reperr_null(create_error(ERR_ALREADYMERGING));
        return;
    }

    if(!dialog_box_start())
        return;

    /* suggest a file to merge with */
    if( !insert_most_recent(&d_user_merge[1].textfield)  ||
        (str_isblank(d_user_merge[0].textfield)  &&  !mystr_set(&d_user_merge[0].textfield, DUMP_FILE_STR)))
        return;

    while(dialog_box(D_USER_MERGE))
    {
        /* open file */
        name = d_user_merge[0].textfield;

        if(str_isblank(name))
        {
            reperr_null(create_error(ERR_BAD_NAME));
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        if(!file_find_on_path_or_relative(buffer, elemof32(buffer), name, currentfilename))
        {
            reperr(create_error(ERR_NOTFOUND), name);
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        mergedict_in = pd_file_open(buffer, file_open_read);

        if(!mergedict_in)
        {
            reperr(create_error(ERR_CANNOTOPEN), name);
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        dialog_box_end();

        /* open user dictionary */
        mdsp->dict = dict_number(d_user_merge[1].textfield, TRUE);

        if(mdsp->dict >= 0)
        {
            /* read words from file adding to user dictionary */

            if(init_box(mdsp, "merging", Merging_STR, TRUE))
            {
                mergedict_start(mdsp);
                return;
            }
        }
        else
            mdsp->res = mdsp->dict;

        mergedict_end(mdsp);

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
*                         Dump dictionary to file
*
******************************************************************************/

static char *      dumpdict_name       = NULL;
static FILE_HANDLE dumpdict_out        = NULL;
static BOOL        dumpdict_file_error;
static char        dumpdict_template[MAX_WORD+1];
static char        dumpdict_wild_string[MAX_WORD+1];

null_event_proto(static, dumpdict_null_handler);

static void
dumpdict_close(
    merge_dump_strukt * mdsp)
{
    mdsp->stopped = TRUE;

    if(dumpdict_out)
    {
        been_error = been_error || dumpdict_file_error;

        if(!been_error)
            file_set_type(dumpdict_out, FILETYPE_TEXT);

        pd_file_close(&dumpdict_out);

        str_clr(&dumpdict_name);

        if(!been_error)
            /* suggest as the file to merge with */
            (void) mystr_set(&d_user_merge[0].textfield, d_user_dump[1].textfield);
    }
}

/* dumpdict complete - tidy up */

static void
dumpdict_end(
    merge_dump_strukt * mdsp)
{
    mdsp->wants_nulls = FALSE;

    Null_EventHandlerRemove(dumpdict_null_handler, mdsp);

    if(dumpdict_file_error)
        reperr(create_error(ERR_CANNOTWRITE), d_user_dump[1].textfield);
    else if(mdsp->res < 0)
        reperr_module(create_error(ERR_SPELL), mdsp->res);

    dumpdict_close(mdsp);

    end_box(mdsp);
}

#define dumpdict_Stop     0
#define dumpdict_Pause    3
#define dumpdict_Continue 4

/* dumpdict upcall processor */

static void
dumpdict_eventhandler(
    dbox d,
    void * handle)
{
    merge_dump_strukt * mdsp = handle;
    dbox_field f = dbox_get(d);

    trace_2(TRACE_APP_PD4, "dumpdict_eventhandler got button %d: dumpdict_out = " PTR_XTFMT, f, report_ptr_cast(dumpdict_out));

    switch(f)
    {
    case dumpdict_Continue:
        /* only resume if paused and not ended */
        if(!mdsp->stopped)
            mdsp->wants_nulls = TRUE;
        break;

    case dumpdict_Pause:
        /* stop null events to dumpdict null processor */
        mdsp->wants_nulls = FALSE;
        break;

    default:
        dumpdict_end(mdsp);
        break;
    }
}

/* dumpdict null processor */

static void
dumpdict_null_core(
    merge_dump_strukt * mdsp)
{
    BOOL was_ctrlflag;
    U32 i;

    trace_0(TRACE_APP_PD4, "dumpdict_null");

    for(i = 1; i < 32; ++i)
    {
        escape_enable();

        mdsp->res = spell_nextword(mdsp->dict, dumpdict_template,
                                   dumpdict_template, dumpdict_wild_string,
                                   &ctrlflag);

        was_ctrlflag = escape_disable_nowinge();

        if(!mdsp->res)
        {
            complete_box(&dumpdict_mds, Dump_STR);

            dumpdict_close(mdsp);

            /* force punter to do explicit end */
            mdsp->wants_nulls = FALSE;
            return;
        }

        if(was_ctrlflag  &&  (mdsp->res > 0))
            mdsp->res = create_error(ERR_ESCAPE);

        if(mdsp->res < 0)
        {
            add_error_to_box(mdsp);
            return;
        }

        /* write to file */
        if( !away_string(dumpdict_template, dumpdict_out)  ||
            !away_eol(dumpdict_out)  )
        {
            dumpdict_file_error = TRUE;
            win_send_close(dbox_syshandle(mdsp->d));
        }

        if(*dumpdict_wild_string)
            break;
    }

    add_word_to_box(&dumpdict_mds, dumpdict_template);
}

null_event_proto(static, dumpdict_null_handler)
{
    merge_dump_strukt * mdsp = (merge_dump_strukt *) p_null_event_block->client_handle;

    switch(p_null_event_block->rc)
    {
    case NULL_QUERY:
        return(mdsp->wants_nulls
                        ? NULL_EVENTS_REQUIRED
                        : NULL_EVENTS_OFF);

    case NULL_EVENT:
        dumpdict_null_core(mdsp);

        return(NULL_EVENT_COMPLETED);

    default:
        return(NULL_EVENT_UNKNOWN);
    }
}

static void
dumpdict_start(
    merge_dump_strukt * mdsp)
{
    mdsp->wants_nulls = TRUE;

    status_assert(Null_EventHandlerAdd(dumpdict_null_handler, mdsp, 0));

    dbox_eventhandler(mdsp->d, dumpdict_eventhandler, mdsp);

    /* all dumping done on null events, button processing on upcalls */
}

/******************************************************************************
*
*  dump dictionary
*
******************************************************************************/

extern void
DumpDictionary_fn(void)
{
    char  buffer[BUF_MAX_PATHSTRING];
    char *name;
    merge_dump_strukt *mdsp = &dumpdict_mds;

    if(mdsp->d)
    {
        reperr_null(create_error(ERR_ALREADYDUMPING));
        return;
    }

    if(!dialog_box_start())
        return;

    /* leave last word template alone */

    /* suggest a file to dump to */
    if( !insert_most_recent(&d_user_dump[2].textfield)  ||
        (str_isblank(d_user_dump[1].textfield)  &&  !mystr_set(&d_user_dump[1].textfield, DUMP_FILE_STR)))
        return;

    if(!dialog_box(D_USER_DUMP))
        return;

    dialog_box_end();

    dumpdict_file_error = FALSE;

    mdsp->dict = open_appropriate_dict(&d_user_dump[2]);

    mdsp->res = (mdsp->dict < 0)
                    ? mdsp->dict
                    : compile_wild_string(mdsp->dict, dumpdict_wild_string, d_user_dump[0].textfield);

    if(mdsp->res < 0)
    {
        reperr_module(create_error(ERR_SPELL), mdsp->res);
        return;
    }

    name = d_user_dump[1].textfield;

    if(str_isblank(name))
    {
        reperr_null(create_error(ERR_BAD_NAME));
        return;
    }

    (void) file_add_prefix_to_name(buffer, elemof32(buffer), name, currentfilename);

    if(!mystr_set(&dumpdict_name, buffer))
        return;

    dumpdict_out = pd_file_open(buffer, file_open_write);

    if(!dumpdict_out)
    {
        reperr(create_error(ERR_CANNOTOPEN), name);
        return;
    }

    if(mdsp->dict >= 0)
    {
        *dumpdict_template = '\0';

        if(init_box(mdsp, "merging", Dumping_STR, TRUE))
        {
            dumpdict_start(mdsp);
            return;
        }
    }
    else
        mdsp->res = mdsp->dict;

    dumpdict_end(mdsp);
}

/******************************************************************************
*
*  lock dictionary
*
******************************************************************************/

extern void
LockDictionary_fn(void)
{
    STATUS status;

    if(!init_dialog_box(D_USER_LOCK))
        return;

    if(!dialog_box_start())
        return;

    if(!insert_most_recent(&d_user_lock[0].textfield))
        return;

    while(dialog_box(D_USER_LOCK))
    {
        status = open_appropriate_dict(&d_user_lock[0]);

        if(status_ok(status))
        {
            const DICT_NUMBER dict = (DICT_NUMBER) status;

            if(status_fail(status = spell_load(dict)))
                status_consume(spell_unlock(dict));
        }

        if(status_fail(status))
        {
            reperr_module(create_error(ERR_SPELL), status);
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* unlock dictionary
*
******************************************************************************/

extern void
UnlockDictionary_fn(void)
{
    S32 res;

    if(!init_dialog_box(D_USER_UNLOCK))
        return;

    if(!dialog_box_start())
        return;

    if(!insert_most_recent(&d_user_unlock[0].textfield))
        return;

    while(dialog_box(D_USER_UNLOCK))
    {
        res = open_appropriate_dict(&d_user_unlock[0]);

        if(res >= 0)
            res = spell_unlock(res);

        if(res < 0)
        {
            reperr_module(create_error(ERR_SPELL), res);
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* set dictionary options
*
******************************************************************************/

extern void
DictionaryOptions_fn(void)
{
    const char *name;
    S32 res;

    if(!init_dialog_box(D_USER_OPTIONS))
        return;

    if(!dialog_box_start())
        return;

    if(!insert_most_recent(&d_user_options[0].textfield))
        return;

    while(dialog_box(D_USER_OPTIONS))
    {
        name = d_user_options[0].textfield;

        if(str_isblank(name))
        {
            reperr_null(create_error(ERR_BAD_NAME));
            if(!dialog_box_can_retry())
                break;
            (void) mystr_set(&d_user_options[0].textfield, USERDICT_STR);
            continue;
        }

        res = dict_number(name, TRUE);

        if(res >= 0)
        {
            trace_2(TRACE_APP_PD4, "spell_setoptions(%d, %8x)", res, (d_user_options[0].option == 'Y') ? DICT_READONLY : 0);
            res = spell_setoptions(res,
                        /* OR */   (d_user_options[0].option == 'Y') ? DICT_READONLY : 0,
                        /* AND */  ~DICT_READONLY);
        }

        if(res < 0)
        {
            reperr_module(create_error(ERR_SPELL), res);
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* display list of opened user dictionaries
*
******************************************************************************/

#define dispdict_Stop     0
#define dispdict_Pause    3
#define dispdict_Continue 4

extern void
DisplayOpenDicts_fn(void)
{
    merge_dump_strukt *mdsp = &browse_mds;
    dbox_field f;

    if(init_box(mdsp, "merging", Opened_user_dictionaries_STR, FALSE))
    {
        S32 i = 1; /* put one line of space at top */
        PC_LIST lptr = first_in_list(&first_user_dict);

        do  {
            if(lptr)
            {
                strcpy(mdsp->words[i], file_leafname((char *) (lptr->value)));
                trace_1(TRACE_APP_PD4, "got user dict %s", mdsp->words[i]);
                lptr = next_in_list(&first_user_dict);
            }
        }
        while(++i < BROWSE_DEPTH);

        fill_box(mdsp->words, NULL, mdsp->d);

        /* rather simple process: no nulls required! */

        while(((f = riscdialog_fillin_for_browse(mdsp->d)) != dbox_CLOSE)  &&
              (f != dbox_OK))
            ;

        end_box(mdsp);
    }
}

/******************************************************************************
*
*  pack dictionary
*
******************************************************************************/

extern void
PackUserDict_fn(void)
{
    S32 res, res1, dict0, dict1;
    char array0[BUF_MAX_PATHSTRING];
    char array1[BUF_MAX_PATHSTRING];
    char *name0, *name1, *leaf;

    if(!dialog_box_start())
        return;

    if(!insert_most_recent(&d_user_pack[0].textfield))
        return;

    while(dialog_box(D_USER_PACK))
    {
        name0 = d_user_pack[0].textfield;
        name1 = d_user_pack[1].textfield;

        if(str_isblank(name0)  ||  str_isblank(name1))
        {
            reperr_null(create_error(ERR_BAD_NAME));
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        /* a better way would be to have a 'read pathname of dict' fn
         * for the case that dict0 is already open as we'd like to create
         * dict1 relative to it. But that's too much hard work so we
         * take the easy way out and always look the pathname up by hand.
        */
        close_user_dictionaries(FALSE);

        if(!add_path_using_dir(array0, elemof32(array0), name0, DICTS_SUBDIR_STR))
        {
            reperr(create_error(ERR_NOTFOUND), name0);
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        name0 = array0;

        if(!file_is_rooted(name1))
        {
            /* use pathname of name0 for name1 if not rooted */
            leaf = file_leafname(name0);
            memcpy32(array1, name0, leaf - name0);
            strcpy(array1 + (leaf - name0), name1);
            name1 = array1;
        }

        /* create second dictionary based on first dictionary */
        res = spell_createdict(name1, name0);

        /* open first dictionary */
        if(res < 0)
            dict0 = 0; /* keep dataflower happy */
        else if((dict0 = dict_number(name0, TRUE)) < 0)
            res = dict0;

        if(res < 0)
            dict1 = 0; /* keep dataflower happy */
        else if((dict1 = dict_number(name1, TRUE)) < 0)
            res = dict1;

        if(res < 0)
        {
            reperr_module(create_error(ERR_SPELL), res);
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        if(res >= 0)
            res = spell_pack(dict0, dict1);

        res1 = close_dict(dict0);

        if(res1 >= 0)
            res1 = close_dict(dict1);

        if(res < 0)
        {
            been_error = FALSE;
            res1 = res;
        }

        if(res1 < 0)
        {
            reperr_module(create_error(ERR_SPELL), res1);
            break;
        }

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/* end of browse.c */
