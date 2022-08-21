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
    DICT_NUMBER dict;

    dbox d;
    HOST_WND window_handle;

    STATUS res;
    S32 sofar;
    BOOL wants_nulls;
    BOOL stopped;

    char (*words) [MAX_WORD+1];
}
merge_dump_strukt;

#define MDS_INIT { -1,  NULL, HOST_WND_NONE,  STATUS_OK, 0, FALSE, FALSE,  NULL }

/*
internal functions
*/

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

#define BROWSE_CENTRE (BROWSE_DEPTH/2 - 1)

static struct ANAGRAM_STATICS
{
    merge_dump_strukt mds;

    BOOL subgrams;
    char last_found[MAX_WORD+1];
    A_LETTER letters[MAX_WORD+1];
}
anagram_statics =
{
    MDS_INIT
};

static struct BROWSE_STATICS
{
    merge_dump_strukt mds;

    char *wild_str;
    char *template;
    char *wild_string;
    BOOL iswild;
    BOOL was_spell_error;
}
browse_statics =
{
    MDS_INIT
};

static struct DUMPDICT_STATICS
{
    merge_dump_strukt mds;

    FILE_HANDLE out;
    char * name;
    BOOL had_file_error;
    char template[MAX_WORD+1];
    char wild_string[MAX_WORD+1];
}
dumpdict_statics =
{
    MDS_INIT
};

static struct MERGEDICT_STATICS
{
    merge_dump_strukt mds;

    FILE_HANDLE in;
    char array[MAX_WORD+1];
}
mergedict_statics =
{
    MDS_INIT
};

/******************************************************************************
*
*  close a user dictionary
*
******************************************************************************/

_Check_return_
static STATUS
close_dict_always(
    _InVal_     DICT_NUMBER dict)
{
    STATUS res;

#if 1
    /* SKS after PD 4.12 26mar92 - treat master dictionary like user dictionaries */
    if(master_dictionary == dict)
        master_dictionary = -1;
#endif

    res = spell_close(dict);

    delete_from_list(&first_user_dict, (S32) dict);

    most_recent = -1;

    return(res);
}

_Check_return_
extern STATUS
close_dict(
    _InVal_     DICT_NUMBER dict)
{
    if( (dict == dumpdict_statics.mds.dict)  ||
        (dict == mergedict_statics.mds.dict) ||
        (dict == anagram_statics.mds.dict)   )
    {
        return(create_error(SPELL_ERR_CANTCLOSE));
    }

    return(close_dict_always(dict));
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
        ((mergedict_statics.mds.dict == -1)  &&  (anagram_statics.mds.dict == -1)  &&  (dumpdict_statics.mds.dict == -1)))
    {
        PC_LIST lptr;

        while((lptr = first_in_list(&first_user_dict)) != NULL)
        {
            STATUS res = close_dict_always((S32) lptr->key);

            if(status_fail(res))
            {
                consume_bool(reperr_null(res));
                been_error = FALSE;
            }
        }
    }
}

/******************************************************************************
*
*  compile the wild string - returning whether it is wild
*  all characters for string MUST be valid for given dict
*
******************************************************************************/

static STATUS
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
        while((ch = *ptr++) != CH_NULL)
        {
            if(ch == '^')
            {
                ch = *ptr++;

                switch(ch)
                {
                case CH_NULL:
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

    *to = CH_NULL;
    return(iswild);
}

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
init_box(
    merge_dump_strukt * mdsp,
    const char *dname,
    const char *title,
    BOOL statik)
{
    S32 y;
    dbox d;
    void *core;
    STATUS status;
    char *errorp;

    trace_3(TRACE_APP_PD4, "init_box(" PTR_XTFMT ", %s, static = %s):", report_ptr_cast(mdsp), dname, report_boolstring(statik));

    mdsp->res = STATUS_OK;
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
            consume_bool(reperr(ERR_OUTPUTSTRING, errorp));
        al_ptr_free(core);
        return(FALSE);
    }

    mdsp->d = d;

    mdsp->window_handle = dbox_window_handle(d);

    dbox_setfield(d, browsing_Template, NULLSTR);

    for(y = 0; y < BROWSE_DEPTH; ++y)
    {
        mdsp->words[y][0] = CH_NULL;
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

    mdsp->window_handle = HOST_WND_NONE;
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

/* has writeable template changed? - checked on null events */

static void
browse_null_core(
    merge_dump_strukt * mdsp)
{
    dbox d = mdsp->d;
 /* const HOST_WND window_handle = mdsp->window_handle; */
    char *str = (browse_statics.iswild) ? browse_statics.wild_str : browse_statics.template;
    char array[MAX_WORD+1];

    trace_0(TRACE_APP_PD4, "browse_null()");

    dbox_getfield(d, browsing_Template, array, sizeof(array));

    if(0 != C_stricmp(array, str))
    {
        if(browse_statics.iswild)
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
                mdsp->res = get_and_display_words(TRUE, browse_statics.iswild, mdsp->words,
                                                  browse_statics.wild_string, browse_statics.template,
                                                  browse_statics.wild_str, mdsp->dict,
                                                  d, &browse_statics.was_spell_error);
                if(status_fail(mdsp->res))
                    winx_send_close_window_request(dbox_window_handle(d));
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
    browse_null_core(&browse_statics.mds);
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
                report_boolstring(iswild), wild_string, template);

        if(iswild  ||  ((res = spell_checkword(dict, template)) == 0))
            if((res = spell_nextword(dict, words[BROWSE_CENTRE], words[BROWSE_CENTRE], wild_string, &ctrlflag)) == 0)
                res = spell_prevword(dict, words[BROWSE_CENTRE], template, wild_string, &ctrlflag);

        for(y = BROWSE_CENTRE + 1;
            !ctrlflag  &&  (res >= 0)  &&  (y < BROWSE_DEPTH);
            ++y)
            if(!*words[y-1]  ||  ((res = spell_nextword(dict, words[y], words[y-1], wild_string, &ctrlflag)) <= 0))
                words[y][0] = CH_NULL;
            else if(iswild)
                fill_box(words, wild_str, d);

        for(y = BROWSE_CENTRE-1; !ctrlflag  &&  (y >= 0) &&  (res >= 0); --y)
            if(!*words[y+1]  ||  ((res = spell_prevword(dict, words[y], words[y+1], wild_string, &ctrlflag)) <= 0))
                words[y][0] = CH_NULL;

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
browse_raw_eventhandler_Key_Pressed(
    /*mutated*/ int * const p_event_code,
    /*mutated*/ WimpPollBlock * const p_event_data,
    const WimpKeyPressedEvent * const key_pressed)
{
    int icon_handle = -1;

    switch(key_pressed->key_code)
    {
    case akbd_UpK:
        icon_handle = browsing_ScrollUp;
        break;

    case akbd_DownK:
        icon_handle = browsing_ScrollDown;
        break;

    case akbd_PageUpK:
        icon_handle = browsing_PageUp;
        break;

    case akbd_PageDownK:
        icon_handle = browsing_PageDown;
        break;

    default:
        break;
    }

    if(icon_handle != -1)
    {   /* mutate event for dbox handler */
        *p_event_code = Wimp_EMouseClick;
        p_event_data->mouse_click.icon_handle = icon_handle;
        p_event_data->mouse_click.buttons = Wimp_MouseButtonSelect;

        clearkeyboardbuffer();
    }

    return(FALSE);
}

static BOOL
browse_raw_eventhandler(
    dbox d,
    void * event,
    void * handle)
{
    const int event_code = ((WimpEvent *) event)->event_code;
    WimpPollBlock * const event_data = &((WimpEvent *) event)->event_data;

    UNREFERENCED_PARAMETER(d);
    UNREFERENCED_PARAMETER(handle);

    trace_1(TRACE_APP_PD4, "raw_event_browse got %s", report_wimp_event(event_code, event_data));

    switch(event_code)
    {
    case Wimp_EKeyPressed:
        return(browse_raw_eventhandler_Key_Pressed(&((WimpEvent *) event)->event_code,
                                                   event_data,
                                                   &event_data->key_pressed));

    default:
        return(FALSE);
    }
}

static void
browse_process(void)
{
    merge_dump_strukt * mdsp = &browse_statics.mds;
    dbox d = mdsp->d;
 /* const HOST_WND window_handle = mdsp->window_handle; */
    dbox_field f;
    const DICT_NUMBER dict = mdsp->dict;
    char (*words)[MAX_WORD+1] = mdsp->words;
    S32 i;
    S32 which = -1;        /* which word was clicked on */
    BOOL fillall;
    char array[MAX_WORD+1];

    if(!*browse_statics.template)
    {
        /* start off at 'a' if nothing specified */
        *browse_statics.template   = 'a';
        browse_statics.template[1] = CH_NULL;
    }

    escape_enable();

    mdsp->res = get_and_display_words(TRUE, browse_statics.iswild, words,
                                      browse_statics.wild_string, browse_statics.template,
                                      browse_statics.wild_str, dict,
                                      d,
                                      &browse_statics.was_spell_error);

    if(status_fail(mdsp->res))
    {
        consume_bool(reperr_null(mdsp->res));
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
        BOOL adjust_clicked = riscos_adjust_clicked();

        (void) dbox_adjusthit(&f, browsing_ScrollUp, browsing_ScrollDown, adjust_clicked);
        (void) dbox_adjusthit(&f, browsing_PageUp,   browsing_PageDown,   adjust_clicked);

        fillall = FALSE;

        escape_enable();

        switch(f)
        {
        case browsing_ScrollUp:
            if(*words[BROWSE_CENTRE-1])
            {
                scroll_words_down(words, BROWSE_DEPTH);

                if(!browse_statics.iswild)
                    strcpy(browse_statics.template, words[BROWSE_CENTRE]);

                if(*words[0])
                    mdsp->res = spell_prevword(dict,
                                               words[0],
                                               words[0],
                                               browse_statics.wild_string,
                                               &ctrlflag);
            }
            break;

        case browsing_ScrollDown:
            if(*words[BROWSE_CENTRE+1])
            {
                scroll_words_up(words, BROWSE_DEPTH);

                if(!browse_statics.iswild)
                    strcpy(browse_statics.template, words[BROWSE_CENTRE]);

                if(*mdsp->words[BROWSE_DEPTH-1])
                    mdsp->res = spell_nextword(dict,
                                               words[BROWSE_DEPTH-1],
                                               words[BROWSE_DEPTH-1],
                                               browse_statics.wild_string,
                                               &ctrlflag);
            }
            break;

        case browsing_PageUp:
            if(browse_statics.iswild)
                bleep();
            else
            {
                mdsp->res = 1;

                for(i = 0; (mdsp->res > 0)  &&  (i < BROWSE_DEPTH); ++i)
                {
                    strcpy(array, browse_statics.template);

                    mdsp->res = spell_prevword(dict,
                                               browse_statics.template,
                                               browse_statics.template,
                                               browse_statics.wild_string,
                                               &ctrlflag);

                    if(0 == mdsp->res)
                    {
                        /* restore template if no word found */
                        strcpy(browse_statics.template, array);
                        break;
                    }
                }

                fillall = TRUE;
            }
            break;

        case browsing_PageDown:
            if(browse_statics.iswild)
                bleep();
            else
            {
                mdsp->res = 1;

                for(i = 0; (mdsp->res > 0)  &&  (i < BROWSE_DEPTH); ++i)
                {
                    strcpy(array, browse_statics.template);

                    mdsp->res = spell_nextword(dict,
                                               browse_statics.template,
                                               browse_statics.template,
                                               browse_statics.wild_string,
                                               &ctrlflag);

                    if(0 == mdsp->res)
                    {
                        /* restore template if no word found */
                        strcpy(browse_statics.template, array);
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

        mdsp->res = get_and_display_words(fillall, browse_statics.iswild, words,
                                          browse_statics.wild_string, browse_statics.template,
                                          browse_statics.wild_str, dict,
                                          d,
                                          &browse_statics.was_spell_error);

        if(status_fail(mdsp->res))
        {
            escape_disable();
            break;
        }

    LOOP_EXIT:

        if(escape_disable_nowinge())
        {
            mdsp->res = ERR_ESCAPE;
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
    if(status_ok(mdsp->res)  &&  (f == dbox_OK))
    {
        if(which >= 0)
        {
            strcpy(word_to_insert, words[which]);
        }
        else
        {
            dbox_getfield(d, browsing_Template, browse_statics.template, LIN_BUFSIZ);

            if(CH_NULL == browse_statics.template[0])
            {
                word_to_insert[0] = 'a';
                word_to_insert[1] = browse_statics.template[0];
            }
            else
            {
                strcpy(word_to_insert, browse_statics.iswild ? words[BROWSE_CENTRE] : browse_statics.template);
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

extern S32
browse(
    _InVal_     DICT_NUMBER dict,
    char *wild_str)
{
    merge_dump_strukt * mdsp = &browse_statics.mds;
    char template[LIN_BUFSIZ];
    char wild_string[LIN_BUFSIZ];

    /* compile the wild string */
    *template = CH_NULL;

    if(!wild_str)
        wild_str = NULLSTR;

    browse_statics.iswild = compile_wild_string(dict, wild_string, wild_str);
    if(browse_statics.iswild < 0)
        return(browse_statics.iswild);

    if(!browse_statics.iswild)
    {
        *wild_string = CH_NULL;
        strcpy(template, wild_str);
    }

    *word_to_insert = template[MAX_WORD] = CH_NULL;

    mdsp->dict = dict;

    browse_statics.wild_str = wild_str;
    browse_statics.template = template;
    browse_statics.wild_string = wild_string;

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

static BOOL
browsedictionary_fn_action(
    char * array,
    S32 found_offset)
{
    STATUS res;

    browse_statics.was_spell_error = FALSE;

    res = open_appropriate_dict(&d_user_browse[1]);

    if(status_ok(res))
        res = browse(res, d_user_browse[0].textfield);

    if(status_fail(res))
        return(reperr_null(res));

    if(*word_to_insert && is_current_document())
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

    return(TRUE);
}

static BOOL
browsedictionary_fn_prepare(
    _In_opt_z_  char * str)
{
    false_return(dialog_box_can_start());

    if(NULL != str)
        false_return(mystr_set(&d_user_browse[0].textfield, str));

    d_user_browse[1].option = 'N';

    false_return(insert_most_recent_userdict(&d_user_browse[1].textfield));

    false_return(!err_open_master_dict());

    return(dialog_box_start());
}

extern void
BrowseDictionary_fn(void)
{
    char array[LIN_BUFSIZ];
    S32 found_offset;

    if(!browsedictionary_fn_prepare(
            get_word_from_line(master_dictionary, array, lecpos, &found_offset)))
        return;

    if(!dialog_box(D_USER_BROWSE))
        return;

    dialog_box_end();

    consume_bool(browsedictionary_fn_action(array, found_offset));
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
    merge_dump_strukt * mdsp = &browse_statics.mds;
    dbox_field f;

    if(init_box(mdsp, "merging", Opened_user_dictionaries_STR, FALSE))
    {
        S32 i = 1; /* put one line of space at top */
        PC_LIST lptr = first_in_list(&first_user_dict);

        do  {
            if(NULL != lptr)
            {
                strcpy(mdsp->words[i], file_leafname(lptr->value));
                trace_1(TRACE_APP_PD4, "got user dict %s", mdsp->words[i]);
                lptr = next_in_list(&first_user_dict);
            }
        }
        while(++i < BROWSE_DEPTH);

        fill_box(mdsp->words, NULL, mdsp->d);

        /* rather simple process: no nulls required! */

        while( ((f = riscdialog_fillin_for_browse(mdsp->d)) != dbox_CLOSE)  &&
               (f != dbox_OK) )
        {
            /*EMPTY*/
        }

        end_box(mdsp);
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

    *to = CH_NULL;
    return(TRUE);
}

static void
add_word_to_box(
    merge_dump_strukt * mdsp,
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

null_event_proto(static, anagram_null_handler);

/* mergedict complete - tidy up */

static void
anagram_end(
    merge_dump_strukt * mdsp)
{
    mdsp->wants_nulls = FALSE;

    Null_EventHandlerRemove(anagram_null_handler, mdsp);

    if(status_fail(mdsp->res))
        reperr_null(mdsp->res);

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

static void
anagram_null_stopping(
    merge_dump_strukt * mdsp)
{
    complete_box(mdsp, anagram_statics.subgrams ? Subgrams_STR : Anagrams_STR);

    /* force punter to do explicit end */
    mdsp->wants_nulls = FALSE;
}

static void
anagram_null_core(
    merge_dump_strukt * mdsp)
{
    char newword[MAX_WORD+1];
    STATUS res1;

    trace_0(TRACE_APP_PD4, "anagram_null()");

    if(a_fillout(anagram_statics.letters, newword, anagram_statics.last_found, anagram_statics.subgrams))
    {
        /* SKS - stop us from getting bad words */
        if(spell_valid_1(mdsp->dict, *newword))
        {
            mdsp->res = spell_checkword(mdsp->dict, newword);

            if(status_fail(mdsp->res))
            {
                add_error_to_box(mdsp);
                return;
            }

            /* don't like most single letter words */
            if( (0 != mdsp->res)  &&
                ((newword[1])  || (*newword == 'i') ||  (*newword == 'a'))
                )
                add_word_to_box(mdsp, newword);
        }

        escape_enable();

        mdsp->res = spell_nextword(mdsp->dict, anagram_statics.last_found, newword, NULL, &ctrlflag);

        res1 = escape_disable_nowinge();

        if(status_fail(res1) && status_ok(mdsp->res))
            mdsp->res = res1;

        if(status_fail(mdsp->res))
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

    if(0 == mdsp->res)
    {
        anagram_null_stopping(mdsp);
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

static BOOL
anagram_start(
    merge_dump_strukt * mdsp)
{
    mdsp->wants_nulls = TRUE;

    status_assert(Null_EventHandlerAdd(anagram_null_handler, mdsp, 0));

    dbox_eventhandler(mdsp->d, anagram_eventhandler, mdsp);

    /* all anagram searching done on null events, button processing on upcalls */
    return(TRUE);
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

static BOOL
anagrams_fn_action(
    _InVal_     BOOL subgrams)
{
    merge_dump_strukt * mdsp = &anagram_statics.mds;
    char * word = d_user_anag[0].textfield;
    STATUS res;
    char array[MAX_WORD+1];
    A_LETTER * aptr;
    uchar * from, * to;
    char ch;

    res = open_appropriate_dict(&d_user_anag[1]);

    if(status_ok(res))
    {
        mdsp->dict = (DICT_NUMBER) res;

        res = compile_wild_string(mdsp->dict, array, word);
    }

    if(status_fail(res))
        return(reperr_null(res));

    /* sort letters into order */
    from = word;
    *array = CH_NULL;
    while((ch = *from++) != CH_NULL)
    {
        ch = spell_tolower(mdsp->dict, ch);

        for(to = array; ; to++)
        {
            if(!*to  ||  (*to > ch))
            {
                memmove32(to + 1, to, strlen32p1((char *) to));
                *to = ch;
                break;
            }
        }
    }

    trace_1(TRACE_APP_PD4, "array=_%s_", array);

    /* copy letters into letters array */
    from = array;
    aptr = anagram_statics.letters;
    do  {
        ch = *from++;
        aptr++->letter = ch;
    }
    while(CH_NULL != ch);

    *anagram_statics.last_found = CH_NULL;

    anagram_statics.subgrams = subgrams;

    if(init_box(mdsp, "anagram", subgrams ? Subgrams_STR : Anagrams_STR, TRUE))
    {
        dbox_setfield(mdsp->d, browsing_Template, d_user_anag[0].textfield);

        return(anagram_start(mdsp));
    }

    anagram_end(mdsp);

    return(FALSE);
}

/******************************************************************************
*
* anagrams
*
******************************************************************************/

static BOOL
anagrams_fn_core(
    _InVal_     BOOL subgrams)
{
    const char * const word = d_user_anag[0].textfield;

    if(str_isblank(word)  ||  (strlen(word) > MAX_WORD))
    {
        consume_bool(reperr_null(SPELL_ERR_BADWORD));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    dialog_box_end();

    return(anagrams_fn_action(subgrams));
}

static BOOL
anagrams_fn_prepare(void)
{
    merge_dump_strukt * mdsp = &anagram_statics.mds;

    if(mdsp->window_handle)
        return(reperr_null(anagram_statics.subgrams ? ERR_ALREADYSUBGRAMS : ERR_ALREADYANAGRAMS));

    false_return(dialog_box_can_start());

    false_return(insert_most_recent_userdict(&d_user_anag[1].textfield));

    return(dialog_box_start());
}

extern void
Anagrams_fn(void)
{
    if(!anagrams_fn_prepare())
        return;

    while(dialog_box(D_USER_ANAG))
    {
        int core_res = anagrams_fn_core(FALSE);

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
* subgrams
*
******************************************************************************/

extern void
Subgrams_fn(void)
{
    if(!anagrams_fn_prepare())
        return;

    while(dialog_box(D_USER_SUBGRAM))
    {
        int core_res = anagrams_fn_core(TRUE);

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
*                         Dump dictionary to file
*
******************************************************************************/

null_event_proto(static, dumpdict_null_handler);

static void
dumpdict_close(
    merge_dump_strukt * mdsp)
{
    mdsp->stopped = TRUE;

    if(NULL == dumpdict_statics.out)
        return;

    been_error = been_error || dumpdict_statics.had_file_error;

    if(!been_error)
        file_set_type(dumpdict_statics.out, FILETYPE_TEXT);

    pd_file_close(&dumpdict_statics.out);

    str_clr(&dumpdict_statics.name);

    if(!been_error) /* suggest as the file to merge with */
        consume_bool(mystr_set(&d_user_merge[0].textfield, d_user_dump[1].textfield));
}

/* dumpdict complete - tidy up */

static void
dumpdict_end(
    merge_dump_strukt * mdsp)
{
    mdsp->wants_nulls = FALSE;

    Null_EventHandlerRemove(dumpdict_null_handler, mdsp);

    dumpdict_close(mdsp);

    if(dumpdict_statics.had_file_error)
        consume_bool(reperr(ERR_CANNOTWRITE, d_user_dump[1].textfield));
    else if(status_fail(mdsp->res))
        consume_bool(reperr_null(mdsp->res));

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

    trace_2(TRACE_APP_PD4, "dumpdict_eventhandler got button %d: dumpdict_statics.out = " PTR_XTFMT, f, report_ptr_cast(dumpdict_statics.out));

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
dumpdict_null_stopping(
    merge_dump_strukt * mdsp)
{
    complete_box(mdsp, Dump_STR);

    dumpdict_close(mdsp);

    /* force punter to do explicit end */
    mdsp->wants_nulls = FALSE;
}

static void
dumpdict_null_core(
    merge_dump_strukt * mdsp)
{
    BOOL was_ctrlflag;
    const MONOTIME starttime = monotime();
    const MONOTIMEDIFF lengthtime = MONOTIME_VALUE(50);

    trace_0(TRACE_APP_PD4, "dumpdict_null");

    do  {
        escape_enable();

        mdsp->res = spell_nextword(mdsp->dict, dumpdict_statics.template,
                                   dumpdict_statics.template, dumpdict_statics.wild_string,
                                   &ctrlflag);

        was_ctrlflag = escape_disable_nowinge();

        if(0 == mdsp->res)
        {
            dumpdict_null_stopping(mdsp);
            return;
        }

        if(was_ctrlflag  &&  status_ok(mdsp->res))
            mdsp->res = ERR_ESCAPE;

        if(status_fail(mdsp->res))
        {
            add_error_to_box(mdsp);
            return;
        }

        /* write to file */
        if( !away_string(dumpdict_statics.template, dumpdict_statics.out)  ||
            !away_eol(dumpdict_statics.out)  )
        {
            dumpdict_statics.had_file_error = TRUE;
            winx_send_close_window_request(dbox_window_handle(mdsp->d));
        }

        if(*dumpdict_statics.wild_string)
            break;
    }
    while(monotime_diff(starttime) < lengthtime);

    add_word_to_box(&dumpdict_statics.mds, dumpdict_statics.template);
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

static BOOL
dumpdict_start(
    merge_dump_strukt * mdsp)
{
    mdsp->wants_nulls = TRUE;

    status_assert(Null_EventHandlerAdd(dumpdict_null_handler, mdsp, 0));

    dbox_eventhandler(mdsp->d, dumpdict_eventhandler, mdsp);

    /* all dumping done on null events, button processing on upcalls */
    return(TRUE);
}

/******************************************************************************
*
*  dump dictionary
*
******************************************************************************/

static BOOL
dumpdictionary_fn_action(void)
{
    merge_dump_strukt * mdsp = &dumpdict_statics.mds;
    char buffer[BUF_MAX_PATHSTRING];
    char * name = d_user_dump[1].textfield;

    dumpdict_statics.had_file_error = FALSE;

    if(str_isblank(name))
        return(reperr_null(ERR_BAD_NAME));

    (void) file_add_prefix_to_name(buffer, elemof32(buffer), name, currentfilename);

    false_return(mystr_set(&dumpdict_statics.name, buffer));

    if(status_ok(mdsp->res = open_appropriate_dict(&d_user_dump[2])))
    {
        mdsp->dict = (DICT_NUMBER) mdsp->res;

        mdsp->res = compile_wild_string(mdsp->dict, dumpdict_statics.wild_string, d_user_dump[0].textfield);
    }

    if(status_fail(mdsp->res))
        return(reperr_null(mdsp->res));

    dumpdict_statics.out = pd_file_open(buffer, file_open_write);

    if(NULL == dumpdict_statics.out)
        return(reperr(ERR_CANNOTOPEN, buffer));

    if(mdsp->dict >= 0)
    {
        *dumpdict_statics.template = CH_NULL;

        if(init_box(mdsp, "merging", Dumping_STR, TRUE))
            return(dumpdict_start(mdsp));
    }

    dumpdict_end(mdsp);

    return(FALSE);
}

static BOOL
dumpdictionary_fn_prepare(void)
{
    merge_dump_strukt * mdsp = &dumpdict_statics.mds;

    if(mdsp->window_handle)
        return(reperr_null(ERR_ALREADYDUMPING));

    false_return(dialog_box_can_start());

    /* leave last word template alone */

    /* suggest a file to dump to */
    if(str_isblank(d_user_dump[1].textfield))
        false_return(mystr_set(&d_user_dump[1].textfield, DUMP_FILE_STR));

    false_return(insert_most_recent_userdict(&d_user_dump[2].textfield));

    return(dialog_box_start());
}

extern void
DumpDictionary_fn(void)
{
    if(!dumpdictionary_fn_prepare())
        return;

    if(!dialog_box(D_USER_DUMP))
        return;

    dialog_box_end();

    consume_bool(dumpdictionary_fn_action());
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

                *ptr = CH_NULL;

                trace_1(TRACE_APP_PD4, "get_word_from_file: got word '%s'", array);
                return(TRUE);
            }
        }
    }
}

null_event_proto(static, mergedict_null_handler);

static void
mergedict_close(
    merge_dump_strukt * mdsp)
{
    mdsp->stopped = TRUE;

    pd_file_close(&mergedict_statics.in);
}

/* mergedict complete - tidy up */

static void
mergedict_end(
    merge_dump_strukt * mdsp)
{
    mdsp->wants_nulls = FALSE;

    Null_EventHandlerRemove(mergedict_null_handler, mdsp);

    mergedict_close(mdsp);

    if(status_fail(mdsp->res))
        consume_bool(reperr_null(mdsp->res));

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

    trace_2(TRACE_APP_PD4, "mergedict_eventhandler got button %d: mergedict_in = " PTR_XTFMT, f, report_ptr_cast(mergedict_statics.in));

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
    const MONOTIME starttime = monotime();
    const MONOTIMEDIFF lengthtime = MONOTIME_VALUE(50);

    trace_0(TRACE_APP_PD4, "mergedict_null()");

    do  {
        if(!get_word_from_file(mdsp->dict, mergedict_statics.in, mergedict_statics.array))
        {
            complete_box(mdsp, Merge_STR);

            mergedict_close(mdsp);

            /* force punter to do explicit end */
            mdsp->wants_nulls = FALSE;
            return;
        }

        mdsp->res = spell_addword(mdsp->dict, mergedict_statics.array);

        if(status_fail(mdsp->res))
        {
            add_error_to_box(mdsp);
            return;
        }
    }
    while(monotime_diff(starttime) < lengthtime);

    if(0 != mdsp->res)
        add_word_to_box(mdsp, mergedict_statics.array);
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

static BOOL
mergedict_start(
    merge_dump_strukt * mdsp)
{
    mdsp->wants_nulls = TRUE;

    status_assert(Null_EventHandlerAdd(mergedict_null_handler, mdsp, 0));

    dbox_eventhandler(mdsp->d, mergedict_eventhandler, mdsp);

    /* all merging done on null events, button processing on upcalls */
    return(TRUE);
}

/******************************************************************************
*
*  merge file with user dictionary
*
******************************************************************************/

static BOOL
mergefilewithdict_fn_action(void)
{
    merge_dump_strukt * mdsp = &mergedict_statics.mds;

    /* open user dictionary */
    mdsp->res = dict_number(d_user_merge[1].textfield, TRUE);

    if(status_ok(mdsp->res))
    {
        mdsp->dict = (DICT_NUMBER) mdsp->res;

        mdsp->res = STATUS_OK;

        if(init_box(mdsp, "merging", Merging_STR, TRUE))
            return(mergedict_start(mdsp));
    }

    mergedict_end(mdsp);

    return(FALSE);
}

static int
mergefilewithdict_fn_core(void)
{
    const char * const name = d_user_merge[0].textfield;
    char buffer[BUF_MAX_PATHSTRING];
    STATUS res;

    /* open file */
    if(str_isblank(name))
    {
        consume_bool(reperr_null(ERR_BAD_NAME));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    if((res = add_path_or_relative_using_dir(buffer, elemof32(buffer), name, TRUE, DICTS_SUBDIR_STR)) <= 0)
    {
        consume_bool(reperr(res ? res : ERR_NOTFOUND, name));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    mergedict_statics.in = pd_file_open(buffer, file_open_read);

    if(NULL == mergedict_statics.in)
    {
        consume_bool(reperr(ERR_CANNOTOPEN, name));
        return(dialog_box_can_retry() ? 2 /*continue*/ : FALSE);
    }

    dialog_box_end();

    return(mergefilewithdict_fn_action());
}

static BOOL
mergefilewithdict_fn_prepare(void)
{
    merge_dump_strukt * mdsp = &mergedict_statics.mds;

    if(mdsp->window_handle)
        return(reperr_null(ERR_ALREADYMERGING));

    false_return(dialog_box_can_start());

    /* suggest a file to merge with */
    if(str_isblank(d_user_merge[0].textfield))
        false_return(mystr_set(&d_user_merge[0].textfield, DUMP_FILE_STR));

    false_return(insert_most_recent_userdict(&d_user_merge[1].textfield));

    return(dialog_box_start());
}

extern void
MergeFileWithDict_fn(void)
{
    if(!mergefilewithdict_fn_prepare())
        return;

    while(dialog_box(D_USER_MERGE))
    {
        int core_res = mergefilewithdict_fn_core();

        if(2 == core_res)
            continue;

        if(0 == core_res)
            break;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/* end of browse.c */
