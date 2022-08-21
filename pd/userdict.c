/* userdict.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1988-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* User dictionary functions */

/* SKS Sep 2016 split off from browse.c */

#include "common/gflags.h"

#include "datafmt.h"

#include "cmodules/spell.h"

/*
internal functions
*/

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
        if(0 == _stricmp(leaf, file_leafname((const char *) lptr->value)))
            return(most_recent = (S32) lptr->key);
    }

    /* not on list so open it possibly (may even be relative to document) */
    dict = create_error(SPELL_ERR_CANTOPEN);
    if(create)
    {
        if(status_done(add_path_or_relative_using_dir(fulldictname, elemof32(fulldictname), name, TRUE, DICTS_SUBDIR_STR)))
        {
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
        }
    }

    most_recent = -1;
    return(dict);
}

/******************************************************************************
*
* put dictionary name of most recently used dictionary in the field
*
******************************************************************************/

extern BOOL
insert_most_recent_userdict(
    char **field)
{
    if(most_recent >= 0)
    {
        P_LIST p_list = search_list(&first_user_dict, (S32) most_recent);
        PTR_ASSERT(p_list);
        return(mystr_set(field, file_leafname(p_list->value)));
    }

    return(mystr_set(field, USERDICT_STR));
}

/******************************************************************************
*
*  open master (or appropriate) dictionary
*
******************************************************************************/

_Check_return_
extern STATUS
open_appropriate_dict(
    const DIALOG * const dptr)
{
    return(    ((dptr->option == 'N')  ||  str_isblank(dptr->textfield))
                    ? open_master_dict()
                    : dict_number(dptr->textfield, TRUE)
            );
}

_Check_return_
extern STATUS
open_master_dict(void)
{
    PC_U8 name;

    if(master_dictionary < 0)
    {
        name = MASTERDICT_STR;

        /* SKS after PD 4.12 26mar92 - treat master dictionary just like user dictionaries */
        master_dictionary = dict_number(name, TRUE);

        /* don't hint that this is the one to be molested */
        most_recent = -1;
    }

    return(master_dictionary);
}

/*ncr*/
extern BOOL
err_open_master_dict(void)
{
    STATUS res = open_master_dict();

    if(status_fail(res))
        return(!reperr_null(res));

    return(FALSE);
}

/******************************************************************************
*
* close user dictionary
*
******************************************************************************/

static int
closeuserdict_fn_core(void)
{
    char * name = d_user_close[0].textfield;
    STATUS res;

    if(str_isblank(name))
    {
        consume_bool(reperr_null(ERR_BAD_NAME));
        false_return(dialog_box_can_retry());
        false_return(mystr_set(&d_user_close[0].textfield, USERDICT_STR));
        return(2); /*continue*/
    }

    /* check it is open already, but not allowing it to be opened */
    res = dict_number(name, FALSE);

    if(status_fail(res))
        res = STATUS_OK; /* ignore error from dict_number */
    else /*res >= 0*/
        res = close_dict(res);

    if(status_fail(res))
    {
        consume_bool(reperr_null(res));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    return(TRUE);
}

static BOOL
closeuserdict_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(init_dialog_box(D_USER_CLOSE));

    false_return(insert_most_recent_userdict(&d_user_close[0].textfield));

    return(dialog_box_start());
}

extern void
CloseUserDict_fn(void)
{
    if(!closeuserdict_fn_prepare())
        return;

    while(dialog_box(D_USER_CLOSE))
    {
        int core_res = closeuserdict_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
*  create user dictionary
*
******************************************************************************/

static int
createuserdict_fn_core(void)
{
    char * name = d_user_create[0].textfield;
    char * language = d_user_create[1].textfield;
    char buffer[BUF_MAX_PATHSTRING];
    char buffer2[BUF_MAX_PATHSTRING];
    STATUS res;

    if(str_isblank(name))
    {
        consume_bool(reperr_null(ERR_BAD_NAME));
        false_return(dialog_box_can_retry());
        false_return(mystr_set(&d_user_create[0].textfield, USERDICT_STR));
        return(2); /*continue*/
    }

    if(str_isblank(language))
    {
        consume_bool(reperr_null(ERR_BAD_LANGUAGE));
        false_return(dialog_box_can_retry());
        false_return(mystr_set(&d_user_create[1].textfield, DEFAULT_DICTDEFN_FILE_STR));
        return(2); /*continue*/
    }

    /* try to create dictionary in a subdirectory of <Choices$Write>.PipeDream */
    name = add_choices_write_prefix_to_name_using_dir(buffer, elemof32(buffer), name, DICTS_SUBDIR_STR);

    /* Add prefix '<PipeDream$Path>.DictDefn.' to language */
    if((res = add_path_using_dir(buffer2, elemof32(buffer2), language, DICTDEFNS_SUBDIR_STR)) <= 0)
    {
        consume_bool(reperr_null(res ? res : ERR_NOTFOUND));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }
    language = buffer2;

    trace_2(TRACE_APP_PD4, "name='%s', language='%s'", name, language);

    if(TRUE /*checkoverwrite(name)*/)
    {
        res = spell_createdict(name, language);

        if(status_ok(res))
            res = dict_number(name, TRUE);

        if(status_fail(res))
        {
            consume_bool(reperr_null(res));
            return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
        }
    }

    return(TRUE);
}

static BOOL
createuserdict_fn_prepare(void)
{
    LIST_ITEMNO languages_list_key;

    false_return(dialog_box_can_start());

    /* Throw away any already existing language_list
     * then scan the dictionary definitions directory
     * and (re)build the language_list.
     * ridialog will try to retain the user's last chosen language.
    */
    delete_list(&languages_list);

    languages_list_key = 0;
    /* enumerate all files in subdirectory DICTDEFNS_SUBDIR_STR found in the directories listed by the PipeDream$Path variable */
    status_assert(enumerate_dir_to_list(&languages_list, &languages_list_key, DICTDEFNS_SUBDIR_STR, FILETYPE_UNDETERMINED)); /* errors are ignored */

    false_return(mystr_set(&d_user_create[0].textfield, USERDICT_STR));

    /* leave d_user_create[1].textfield alone */

    return(dialog_box_start());
}

extern void
CreateUserDict_fn(void)
{
    if(!createuserdict_fn_prepare())
        return;

    while(dialog_box(D_USER_CREATE))
    {
        int core_res = createuserdict_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

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

static int
deletewordfromdict_fn_core(void)
{
    STATUS res = dict_number(d_user_delete[1].textfield, TRUE);

    if(status_ok(res))
        res = spell_deleteword(res, d_user_delete[0].textfield);

    if(status_fail(res))
    {
        consume_bool(reperr_null(res));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    return(TRUE);
}

static BOOL
deletewordfromdict_fn_prepare(void)
{
    char array[LIN_BUFSIZ];

    false_return(dialog_box_can_start());

    false_return(insert_most_recent_userdict(&d_user_delete[1].textfield));

    false_return(!err_open_master_dict());

    false_return(mystr_set(&d_user_delete[0].textfield,
                           get_word_from_line(master_dictionary, array, lecpos, NULL)));

    return(dialog_box_start());
}

extern void
DeleteWordFromDict_fn(void)
{
    if(!deletewordfromdict_fn_prepare())
        return;

    while(dialog_box(D_USER_DELETE))
    {
        int core_res = deletewordfromdict_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
*  flush all of the user dictionaries
*
******************************************************************************/

static void
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
            reperr_null(res);
            been_error = FALSE;
        }
    }
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

static int
insertwordindict_fn_core(void)
{
    STATUS res = dict_number(d_user_insert[1].textfield, TRUE);

    if(status_ok(res))
    {
        res = spell_addword(res, d_user_insert[0].textfield);

        if(0 == res)
            res = create_error(ERR_WORDEXISTS);
    }

    if(status_fail(res))
    {
        consume_bool(reperr_null(res));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    return(TRUE);
}

static BOOL
insertwordindict_fn_prepare(void)
{
    char array[LIN_BUFSIZ];

    false_return(dialog_box_can_start());

    false_return(insert_most_recent_userdict(&d_user_insert[1].textfield));

    false_return(!err_open_master_dict());

    false_return(mystr_set(&d_user_insert[0].textfield,
                           get_word_from_line(master_dictionary, array, lecpos, NULL)));

    return(dialog_box_start());
}

extern void
InsertWordInDict_fn(void)
{
    if(!insertwordindict_fn_prepare())
        return;

    while(dialog_box(D_USER_INSERT))
    {
        int core_res = insertwordindict_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
*  lock dictionary
*
******************************************************************************/

static int
lockdictionary_fn_core(void)
{
    STATUS status = open_appropriate_dict(&d_user_lock[0]);

    if(status_ok(status))
    {
        const DICT_NUMBER dict = (DICT_NUMBER) status;

        if(status_fail(status = spell_load(dict)))
            status_consume(spell_unlock(dict));
    }

    if(status_fail(status))
    {
        consume_bool(reperr_null(status));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    return(TRUE);
}

static BOOL
lockdictionary_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(init_dialog_box(D_USER_LOCK));

    false_return(insert_most_recent_userdict(&d_user_lock[0].textfield));

    return(dialog_box_start());
}

extern void
LockDictionary_fn(void)
{
    if(!lockdictionary_fn_prepare())
        return;

    while(dialog_box(D_USER_LOCK))
    {
        int core_res = lockdictionary_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

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

static int
unlockdictionary_fn_core(void)
{
    STATUS status = open_appropriate_dict(&d_user_unlock[0]);

    if(status_ok(status))
    {
        const DICT_NUMBER dict = (DICT_NUMBER) status;

        status = spell_unlock(dict);
    }

    if(status_fail(status))
    {
        consume_bool(reperr_null(status));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    return(TRUE);
}

static BOOL
unlockdictionary_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(init_dialog_box(D_USER_UNLOCK));

    false_return(insert_most_recent_userdict(&d_user_unlock[0].textfield));

    return(dialog_box_start());
}

extern void
UnlockDictionary_fn(void)
{
    if(!unlockdictionary_fn_prepare())
        return;

    while(dialog_box(D_USER_UNLOCK))
    {
        int core_res = unlockdictionary_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

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

static int
openuserdict_fn_core(void)
{
    char *name = d_user_open[0].textfield;
    STATUS res;

    if(str_isblank(name))
    {
        consume_bool(reperr_null(ERR_BAD_NAME));
        false_return(dialog_box_can_retry());
        false_return(mystr_set(&d_user_open[0].textfield, USERDICT_STR));
        return(2); /*continue*/
    }

    /* open it, if not already open */
    res = dict_number(name, TRUE);

    if(status_fail(res))
    {
        consume_bool(reperr_null(res));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    return(TRUE);
}

static BOOL
openuserdict_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(init_dialog_box(D_USER_OPEN));

    return(dialog_box_start());
}

extern void
OpenUserDict_fn(void)
{
    if(!openuserdict_fn_prepare())
        return;

    while(dialog_box(D_USER_OPEN))
    {
        int core_res = openuserdict_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

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

static int
dictionaryoptions_fn_core(void)
{
    const char * name = d_user_options[0].textfield;
    S32 res;

    if(str_isblank(name))
    {
        consume_bool(reperr_null(ERR_BAD_NAME));
        false_return(dialog_box_can_retry());
        false_return(mystr_set(&d_user_options[0].textfield, USERDICT_STR));
        return(2); /*continue*/
    }

    res = dict_number(name, TRUE);

    if(status_ok(res))
    {
        trace_2(TRACE_APP_PD4, "spell_setoptions(%d, %8x)", res, (d_user_options[0].option == 'Y') ? DICT_READONLY : 0);
        res = spell_setoptions(res,
                    /* OR */   (d_user_options[0].option == 'Y') ? DICT_READONLY : 0,
                    /* AND */  ~DICT_READONLY);
    }

    if(status_fail(res))
    {
        consume_bool(reperr_null(res));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    return(TRUE);
}

static BOOL
dictionaryoptions_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(init_dialog_box(D_USER_OPTIONS));

    false_return(insert_most_recent_userdict(&d_user_options[0].textfield));

    return(dialog_box_start());
}

extern void
DictionaryOptions_fn(void)
{
    if(!dictionaryoptions_fn_prepare())
        return;

    while(dialog_box(D_USER_OPTIONS))
    {
        int core_res = dictionaryoptions_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
*  pack dictionary
*
******************************************************************************/

static int
packuserdict_fn_core(void)
{
    const char * name0 = d_user_pack[0].textfield;
    const char * name1 = d_user_pack[1].textfield;
    S32 res, res1, dict0, dict1;
    char buffer0[BUF_MAX_PATHSTRING];
    char buffer1[BUF_MAX_PATHSTRING];
    char * leaf;

    if(str_isblank(name0)  ||  str_isblank(name1))
    {
        reperr_null(ERR_BAD_NAME);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    /* a better way would be to have a 'read pathname of dict' fn
     * for the case that dict0 is already open as we'd like to create
     * dict1 relative to it. But that's too much hard work so we
     * take the easy way out and always look the pathname up by hand.
    */
    close_user_dictionaries(FALSE);

    if((res = add_path_using_dir(buffer0, elemof32(buffer0), name0, DICTS_SUBDIR_STR)) <= 0)
    {
        consume_bool(reperr(res ? res : ERR_NOTFOUND, name0));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    name0 = buffer0;

    if(!file_is_rooted(name1))
    {
        /* use pathname of name0 for name1 if not rooted */
        leaf = file_leafname(name0);
        memcpy32(buffer1, name0, leaf - name0);
        strcpy(buffer1 + (leaf - name0), name1);
        name1 = buffer1;
    }

    /* create second dictionary based on first dictionary */
    res = spell_createdict(name1, name0);

    /* open first dictionary */
    if(status_fail(res))
        dict0 = 0; /* keep dataflower happy */
    else if((dict0 = dict_number(name0, TRUE)) < 0)
        res = dict0;

    if(status_fail(res))
        dict1 = 0; /* keep dataflower happy */
    else if((dict1 = dict_number(name1, TRUE)) < 0)
        res = dict1;

    if(status_fail(res))
    {
        reperr_null(res);
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    if(status_ok(res))
        res = spell_pack(dict0, dict1);

    res1 = close_dict(dict0);

    if(status_ok(res1))
        res1 = close_dict(dict1);

    if(status_fail(res))
    {
        been_error = FALSE;
        res1 = res;
    }

    if(status_fail(res1))
        return(reperr_null(res1));

    return(TRUE);
}

static BOOL
packuserdict_fn_prepare(void)
{
    false_return(dialog_box_can_start());

    false_return(insert_most_recent_userdict(&d_user_pack[0].textfield));

    return(dialog_box_start());
}

extern void
PackUserDict_fn(void)
{
    if(!packuserdict_fn_prepare())
        return;

    while(dialog_box(D_USER_PACK))
    {
        int core_res = packuserdict_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/* end of userdict.c */
