/* riscmenu.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* RISC OS specific menu code for PipeDream */

/* Stuart K. Swales 05 Jul 1989 */

#include "common/gflags.h"

#include "datafmt.h"

#if !RISCOS
#    error    This module can only be used to build RISC OS code
#endif

/* external header file */
#include "riscmenu.h"

#include "riscos_x.h"
#include "riscdraw.h"
#include "pd_x.h"
#include "ridialog.h"

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

#ifndef __cs_event_h
#include "cs-event.h"
#endif

#ifndef __cs_dbox_h
#include "cs-dbox.h"
#endif

#ifndef __fontlxtr_h
#include "cmodules/riscos/fontlxtr.h"
#endif

#ifndef __fontselect_h
#include "fontselect.h"
#endif

#include "colh_x.h"

/* menu offset for no selection */

#define mo_noselection 0

/*
internal functions
*/

static BOOL
decodesubmenu(
    const MENU_HEAD * mhptr,
    S32 offset,
    S32 nextoffset,
    BOOL submenurequest);

static void
encodesubmenu(
    MENU_HEAD * mhptr,
    BOOL short_m);

static void
goto_dep_or_sup_slot(
    _InRef_     PC_EV_SLR slrp,
    S32 get_deps,
    _InVal_     enum EV_DEPSUP_TYPES category,
    S32 itemno);

static S32 riscmenu__was_main = FALSE;

/******************************************************************************
*
* icon bar menu definition (shared globals)
*
******************************************************************************/

static menu iconbar_menu = NULL;

enum MO_ICONBAR_OFFSETS /* see strings.c iconbar_menu_entries */
{
    mo_iconbar_info      = 1,
    mo_iconbar_help,
    mo_iconbar_documents,
    mo_iconbar_loadtemplate,
    mo_iconbar_choices,
    mo_iconbar_quit

,   mo_iconbar_trace
};

#if TRACE_ALLOWED
static const char iconbar_menu_entries_trace[] = ",Trace";
#endif

/* iconbar 'Documents' submenu */

static menu    iw_menu            = NULL;
static DOCNO * iw_menu_array      = NULL;
static S32     iw_menu_array_upb  = 0;

#define mo_iw_dynamic  1

#define iw_menu_MAXWIDTH 96

/******************************************************************************
*
* icon bar MENU maker - called when MENU pressed on an icon
*
******************************************************************************/

static menu
iconbar_menu_maker(
    void *handle)
{
    S32 num_documents = 0;

    IGNOREPARM(handle);            /* this handle is useless */

    trace_0(TRACE_APP_PD4, "iconbar_menu_maker()");

    /* not the main window menu */
    riscmenu__was_main = FALSE;

    /* create submenu if needed */

    if(!event_is_menu_being_recreated())
        {
        trace_0(TRACE_APP_PD4, "dispose of old & then create new documents menu list");

        /* ensure font selector goes bye-bye */
        pdfontselect_finalise(TRUE);

        menu_dispose(&iw_menu, 0);
        iw_menu = NULL;

            {
            S32 i;
            S32 max_i;
            P_DOCU p_docu;

            for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
            {
                num_documents++;
            }

            /* enlarge array as needed */
            if(num_documents != 0)
                {
                if(num_documents > iw_menu_array_upb)
                    {
                    STATUS status;
                    DOCNO * ptr;

                    if(NULL == (ptr = al_ptr_realloc_elem(DOCNO, iw_menu_array, num_documents, &status)))
                        reperr_null(status);

                    if(ptr)
                        {
                        iw_menu_array     = ptr;
                        iw_menu_array_upb = num_documents;
                        }

                    max_i = iw_menu_array_upb;
                    }
                else
                    max_i = num_documents;

                for(p_docu = first_document(), i = 0; (NO_DOCUMENT != p_docu) && (i < max_i); p_docu = next_document(p_docu))
                    {
                    PCTSTR name = riscos_obtainfilename(p_docu->Xcurrentfilename);
                    U8Z tempstring[256];
                    U32 len = strlen32(name);

                    if(len > iw_menu_MAXWIDTH)
                        {
                        xstrkpy(tempstring, elemof32(tempstring), iw_menu_prefix);
                        xstrkat(tempstring, elemof32(tempstring), name + (len - iw_menu_MAXWIDTH) + strlen32(iw_menu_prefix));
                        }
                    else
                        xstrkpy(tempstring, elemof32(tempstring), name);

                    /* create/extend menu 'unparsed' cos buffer may contain ',' */
                    if(iw_menu == NULL)
                        {
                        if((iw_menu = menu_new_unparsed(iw_menu_title, tempstring)) != NULL)
                            iw_menu_array[i++] = p_docu->docno;
                        }
                    else
                        {
                        menu_extend_unparsed(&iw_menu, tempstring);
                        iw_menu_array[i++] = p_docu->docno;
                        }
                    }

                (void) menu_submenu(iconbar_menu, mo_iconbar_documents, iw_menu);
                }
            }
        }

    /* encode menus */

#if TRACE_ALLOWED
    if(trace_is_enabled())
        menu_setflags(iconbar_menu, mo_iconbar_trace, trace_is_on(), FALSE);
#endif

    /* grey out 'Documents' submenu if not there */
    menu_setflags(iconbar_menu, mo_iconbar_documents, FALSE, (iw_menu == NULL));

    /* encode 'Choices' submenu */
    if(iconbar_headline[0].m)
        encodesubmenu(&iconbar_headline[0], short_menus());

#if 0 /* this is now encoded as part of 'Choices' submenu */
    /* grey out 'Save choices' if not usable because no specified document */
    menu_setflags(iconbar_menu, mo_iconbar_savechoices, FALSE, (NO_DOCUMENT == find_document_with_input_focus()));
#endif

    trace_1(TRACE_APP_PD4, "iconbar menus encoded: returning menu &%p", report_ptr_cast(iconbar_menu));

    return(iconbar_menu);
}

#define CLICK_GIVES_DIALOG TRUE

/******************************************************************************
*
* icon bar MENU selection - called when selection is made from icon MENU
*
******************************************************************************/

static BOOL
iconbar_menu_handler(
    void *handle,
    char *hit,
    BOOL submenurequest)
{
    S32 selection      = hit[0];
    S32 subselection   = hit[1];
    BOOL processed      = TRUE;

    trace_3(TRACE_APP_PD4, "iconbar_menu_handler([%d][%d]): submenurequest = %s",
            selection, subselection, trace_boolstring(submenurequest));

    IGNOREPARM(handle);        /* this handle is useless */

    switch(selection)
        {
        case mo_iconbar_info:
            if(CLICK_GIVES_DIALOG  ||  submenurequest)
                riscdialog_execute(dproc_aboutprog,
                                   "aboutprog", d_about, D_ABOUT);
            /* ignore clicks on 'Info' entry if !CLICK_GIVES_DIALOG */
            break;

        case mo_iconbar_help:
            internal_process_command(N_Help);
            break;

        case mo_iconbar_documents:
            switch(subselection)
                {
                case mo_noselection:
                    /* ignore clicks on 'documents' entry */
                    processed = FALSE;
                    break;

                default:
                    front_document_using_docno(iw_menu_array[subselection - mo_iw_dynamic]);
                    break;
                }
            break;

        case mo_iconbar_loadtemplate:
            switch(subselection)
                {
                case mo_noselection:
                    if(!submenurequest)
                        application_process_command(N_NewWindow);
                    else
                        application_process_command(N_LoadTemplate);
                    break;

                default:
                    processed = FALSE;
                    break;
                }
            break;

        case mo_iconbar_choices:
            switch(subselection)
                {
                case mo_noselection:
                    /* ignore clicks on 'choices' entry */
                    processed = FALSE;
                    break;

                default:
                    processed = decodesubmenu(&iconbar_headline[0],
                                              subselection,
                                              hit[2],
                                              submenurequest);
                    break;
                }
            break;

#if 0
        case mo_iconbar_savechoices:
            {
            /* can only save choices from a specified document */
            P_DOCU p_docu;

            if(NO_DOCUMENT != (p_docu = find_document_with_input_focus()))
                {
                select_document(p_docu);
                application_process_command(N_SaveChoices);
                }

            break;
            }
#endif

        case mo_iconbar_quit:
            application_process_command(N_Quit);
            break;

#if TRACE_ALLOWED
        case mo_iconbar_trace:
            if(trace_is_on())
                trace_off();
            else
                trace_on();
            break;
#endif

        default:
            trace_1(TRACE_APP_PD4, "unprocessed iconbar menu hit %d", hit[0]);
            break;
        }

    return(processed);
}

/******************************************************************************
*
* function selector menu structure
*
******************************************************************************/

#define MENU_FUNCTION_SLOTINFO          1
#define MENU_FUNCTION_DBASE             2
#define MENU_FUNCTION_DATE              3
#define MENU_FUNCTION_FINANCIAL         4
#define MENU_FUNCTION_LOOKUP            5
#define MENU_FUNCTION_STRING            6
#define MENU_FUNCTION_COMPLEX           7
#define MENU_FUNCTION_MATHS             8
#define MENU_FUNCTION_MATRIX            9
#define MENU_FUNCTION_STAT              10
#define MENU_FUNCTION_TRIG              11
#define MENU_FUNCTION_MISC              12
#define MENU_FUNCTION_CONTROL           13
#define MENU_FUNCTION_CUSTOM            14
#define MENU_FUNCTION_NAMES             15
#define MENU_FUNCTION_DEFINENAME        16
#define MENU_FUNCTION_EDITNAME          17

#define MENU_FUNCTION_NUMINFO_SLOTVALUE 1
#define MENU_FUNCTION_NUMINFO_SUPSLR    2
#define MENU_FUNCTION_NUMINFO_SUPRANGE  3
#define MENU_FUNCTION_NUMINFO_SUPNAME   4
#define MENU_FUNCTION_NUMINFO_DEPSLR    5
#define MENU_FUNCTION_NUMINFO_DEPRANGE  6
#define MENU_FUNCTION_NUMINFO_DEPNAME   7

#define MENU_FUNCTION_TEXTINFO_COMPILE  1
#define MENU_FUNCTION_TEXTINFO_DEPSLR   2
#define MENU_FUNCTION_TEXTINFO_DEPRANGE 3
#define MENU_FUNCTION_TEXTINFO_DEPNAME  4

static menu menu_function           = NULL;     /* Top menu */

static menu menu_function_dbase     = NULL;     /* Menus of evaluator functions */
static menu menu_function_date      = NULL;
static menu menu_function_financial = NULL;
static menu menu_function_lookup    = NULL;
static menu menu_function_string    = NULL;
static menu menu_function_complex   = NULL;
static menu menu_function_maths     = NULL;
static menu menu_function_matrix    = NULL;
static menu menu_function_stat      = NULL;
static menu menu_function_trig      = NULL;
static menu menu_function_misc      = NULL;
static menu menu_function_control   = NULL;
static menu menu_function_custom    = NULL;
static menu menu_function_names     = NULL;
/*                        definename is a dbox */
static menu menu_function_editname  = NULL;

static menu menu_function_numinfo   = NULL;
static menu menu_function_textinfo  = NULL;

static menu menu_function_numinfo_slotvalue = NULL;
static menu menu_function_numinfo_supslr    = NULL;
static menu menu_function_numinfo_suprange  = NULL;
static menu menu_function_numinfo_supname   = NULL;
static menu menu_function_numinfo_depslr    = NULL;
static menu menu_function_numinfo_deprange  = NULL;
static menu menu_function_numinfo_depname   = NULL;

static EV_SLR menu_function_slr;

extern void
function__event_menu_maker(void)
{
    if(!menu_function)
        menu_function = menu_new_c(function_menu_title, function_menu_entries);
}

static void
createfunctionsubmenu(
    S32 parentposition,
    menu *submenup,
    PC_U8 submenutitle,
    _InVal_     enum EV_RESOURCE_TYPES submenucontents_category)
{
    menu parentmenu = menu_function; /* used to be a parameter, but since ALL callers passed 'menu_function' I removed it */

    BOOL localsearch     = (parentposition == MENU_FUNCTION_EDITNAME);
    BOOL entryhassubmenu = (parentposition == MENU_FUNCTION_EDITNAME);

    if(!(*submenup))
        {
        menu        submenu = NULL;
        EV_RESOURCE resource;
        EV_OPTBLOCK optblock;
        S32         itemno;
        char        namebuf[BUF_MAX_PATHSTRING + BUF_EV_INTNAMLEN];  /* '>' + name + term */
        char        argbuf[256];        /* size of linbuf or something???*/
        S32         argcount;
        EV_DOCNO    cur_docno;
        char       *insertpoint = namebuf;

        trace_1(TRACE_APP_EXPEDIT, "Creating '%s' submenu", submenutitle);

        if(entryhassubmenu)
            {
            namebuf[0]  = '>';
            insertpoint = namebuf + 1;
            }

        cur_docno = (EV_DOCNO) current_docno();

        ev_set_options(&optblock, cur_docno);

        if(localsearch)
            ev_enum_resource_init(&resource, submenucontents_category, cur_docno, cur_docno, &optblock);
        else
            ev_enum_resource_init(&resource, submenucontents_category, DOCNO_NONE, cur_docno, &optblock);

        for(itemno = -1;
            (ev_enum_resource_get(&resource, &itemno, insertpoint, (BUF_MAX_PATHSTRING + EV_INTNAMLEN), argbuf, 255, &argcount) >= 0);
            itemno = -1
           )
            {
            trace_1(TRACE_APP_EXPEDIT, "  %s", namebuf);

            if(entryhassubmenu)
                {
                /* don't create/extend menu 'unparsed' cos buffer does contain '>' */
                if(submenu == NULL)
                    submenu = menu_new_c(submenutitle, namebuf);
                else
                    menu_extend(submenu, namebuf);
                }
            else
                {
                /* create/extend menu 'unparsed' cos buffer may contain ',' (paranoia here) */
                if(submenu == NULL)
                    submenu = menu_new_unparsed(submenutitle, namebuf);
                else
                    menu_extend_unparsed(&submenu, namebuf);
                }
            }

        *submenup = submenu;
        menu_submenu(parentmenu, parentposition, submenu);
        menu_setflags(parentmenu, parentposition, FALSE /*not ticked*/, (submenu == NULL));
        }
}

static void
substitute_spaces(
    _In_z_      P_U8Z buf,
    _InVal_     U8 replacement_ch)
{
    P_U8Z ptr = buf;

    for(ptr = buf; CH_NULL != *ptr; ++ptr)
    {
        if(CH_SPACE == *ptr)
            *ptr = replacement_ch;
    }
}

#if TRUE
static void
create_sup_dep_submenu(
    S32 parentposition,
    menu *submenup,
    PC_U8 submenutitle,
    _InVal_     enum EV_DEPSUP_TYPES submenucontents_category,
    S32 get_deps,
    menu parentmenu)
{
    P_EV_SLR slrp = &menu_function_slr;

    if(!(*submenup))
        {
        menu        submenu = NULL;
        EV_DOCNO    cur_docno;
        EV_OPTBLOCK optblock;
        EV_DEPSUP   depsup;
        S32         itemno;
        EV_DATA     evdata;
        P_SLOT      p_slot;
        char        namebuf[EV_MAX_IN_LEN + 1];         /* Large buffer cos slot may be external */
        char        valubuf[LIN_BUFSIZ + 1];

        cur_docno = (EV_DOCNO) current_docno();

        ev_set_options(&optblock, cur_docno);

        ev_enum_dep_sup_init(&depsup, slrp, submenucontents_category, get_deps);

        for(itemno = -1;
            (ev_enum_dep_sup_get(&depsup, &itemno, &evdata) >= 0);
            itemno = -1
           )
            {
            ev_decode_data(namebuf, cur_docno, &evdata, &optblock);

            if(evdata.did_num == RPN_DAT_SLR)
                {
                p_slot = travel_externally(evdata.arg.slr.docno,
                                          (COL) evdata.arg.slr.col,
                                          (ROW) evdata.arg.slr.row);
                if(p_slot)
                    {
                    (void) expand_slot(evdata.arg.slr.docno, p_slot, (ROW) evdata.arg.slr.row, valubuf, LIN_BUFSIZ,
                                       DEFAULT_EXPAND_REFS /*expand_refs*/, TRUE /*expand_ats*/, TRUE /*expand_ctrl*/,
                                       FALSE /*allow_fonty_result*/, FALSE /*cff*/);

                    xstrkat(namebuf, elemof32(namebuf), " ");
                    xstrkat(namebuf, elemof32(namebuf), valubuf); /* plain non-fonty string */

                    substitute_spaces(namebuf, 0xA0);
                    }
                }

            /* create/extend menu 'unparsed' cos namebuf may contain ',' */
            if(submenu == NULL)
                submenu = menu_new_unparsed(submenutitle, namebuf);
            else
                menu_extend_unparsed(&submenu, namebuf);
            }

        *submenup = submenu;
        menu_submenu(parentmenu, parentposition, submenu);
        menu_setflags(parentmenu, parentposition, FALSE /*not ticked*/, (submenu == NULL));
        }
}
#endif

extern menu
function__event_menu_filler(
    void *handle)
{
    DOCNO old_docno = current_docno();

    /* ensure font selector goes bye-bye */
    pdfontselect_finalise(TRUE);

    (void) select_document_using_callback_handle(handle);

    if(menu_function)
        {
#if TRUE
        char entry[256];
        char coord[256];                        /* textual form of cur(col,row) is fairly short */

        (void) write_ref(coord, elemof32(coord), current_docno(), curcol, currow); /* always current doc */

        (void) xsnprintf(entry, elemof32(entry), "Slot '%s'", coord);

        if(strlen32(entry) > 12)
            (void) xsnprintf(entry, elemof32(entry), "'%s'", coord);

        if(strlen32(entry) > 12)
            (void) xsnprintf(entry, elemof32(entry), "Slot info");

        menu_entry_changetext(menu_function, MENU_FUNCTION_SLOTINFO, entry);
#endif

        /* createfunctionsubmenu only creates and attachs a submenu if it doesn't already exist, so all   */
        /* submenus except custom and names are created once only when the function menu is first invoked */

      /*createfunctionsubmenu(S32 parentposition    , menu *submenup          , char *submenutitle       , S32 submenucontents)*/

        createfunctionsubmenu(MENU_FUNCTION_DBASE    , &menu_function_dbase    , menu_function_database_title , EV_RESO_DATABASE);
        createfunctionsubmenu(MENU_FUNCTION_DATE     , &menu_function_date     , menu_function_date_title     , EV_RESO_DATE);
        createfunctionsubmenu(MENU_FUNCTION_FINANCIAL, &menu_function_financial, menu_function_financial_title, EV_RESO_FINANCE);
        createfunctionsubmenu(MENU_FUNCTION_LOOKUP   , &menu_function_lookup   , menu_function_lookup_title   , EV_RESO_LOOKUP);
        createfunctionsubmenu(MENU_FUNCTION_STRING   , &menu_function_string   , menu_function_string_title   , EV_RESO_STRING);
        createfunctionsubmenu(MENU_FUNCTION_COMPLEX  , &menu_function_complex  , menu_function_complex_title  , EV_RESO_COMPLEX);
        createfunctionsubmenu(MENU_FUNCTION_MATHS    , &menu_function_maths    , menu_function_maths_title    , EV_RESO_MATHS);
        createfunctionsubmenu(MENU_FUNCTION_MATRIX   , &menu_function_matrix   , menu_function_matrix_title   , EV_RESO_MATRIX);
        createfunctionsubmenu(MENU_FUNCTION_STAT     , &menu_function_stat     , menu_function_stat_title     , EV_RESO_STATS);
        createfunctionsubmenu(MENU_FUNCTION_TRIG     , &menu_function_trig     , menu_function_trig_title     , EV_RESO_TRIG);
        createfunctionsubmenu(MENU_FUNCTION_MISC     , &menu_function_misc     , menu_function_misc_title     , EV_RESO_MISC);
        createfunctionsubmenu(MENU_FUNCTION_CONTROL  , &menu_function_control  , menu_function_control_title  , EV_RESO_CONTROL);

        /* custom and names must be destroyed and recreated everytime */

        if(menu_function_custom)
            {
            menu_submenu(menu_function, MENU_FUNCTION_CUSTOM, NULL);
            menu_dispose(&menu_function_custom, 0);
            menu_function_custom = NULL;
            }
        createfunctionsubmenu(MENU_FUNCTION_CUSTOM   , &menu_function_custom   , menu_function_custom_title   , EV_RESO_CUSTOM);

        if(menu_function_names)
            {
            menu_submenu(menu_function, MENU_FUNCTION_NAMES, NULL);
            menu_dispose(&menu_function_names, 0);
            menu_function_names = NULL;
            }
        createfunctionsubmenu(MENU_FUNCTION_NAMES    , &menu_function_names    , menu_function_names_title    , EV_RESO_NAMES);

        if(menu_function_editname)
            {
            menu_submenu(menu_function, MENU_FUNCTION_EDITNAME, NULL);
            menu_dispose(&menu_function_editname, 0);
            menu_function_editname = NULL;
            }
        createfunctionsubmenu(MENU_FUNCTION_EDITNAME , &menu_function_editname , Edit_name_STR                , EV_RESO_NAMES);

        /* The slot info entry may already have a numinfo or textinfo menu structure attached, if so destroy it, */
        /* then create and attach a structure appropriate to slot (curcol,currow).                               */

        if(menu_function_numinfo)
            {
            menu_submenu(menu_function, MENU_FUNCTION_SLOTINFO, NULL);
            menu_dispose(&menu_function_numinfo, 0);
            menu_function_numinfo = NULL;

            menu_dispose(&menu_function_numinfo_slotvalue, 0);
            menu_function_numinfo_slotvalue = NULL;

            menu_dispose(&menu_function_numinfo_depslr, 0);
            menu_function_numinfo_depslr = NULL;

            menu_dispose(&menu_function_numinfo_deprange, 0);
            menu_function_numinfo_deprange = NULL;

            menu_dispose(&menu_function_numinfo_depname, 0);
            menu_function_numinfo_depname = NULL;

            menu_dispose(&menu_function_numinfo_supslr, 0);
            menu_function_numinfo_supslr = NULL;

            menu_dispose(&menu_function_numinfo_suprange, 0);
            menu_function_numinfo_suprange = NULL;

            menu_dispose(&menu_function_numinfo_supname, 0);
            menu_function_numinfo_supname = NULL;
            }

        if(menu_function_textinfo)
            {
            menu_submenu(menu_function, MENU_FUNCTION_SLOTINFO, NULL);
            menu_dispose(&menu_function_textinfo, 0);
            menu_function_textinfo = NULL;

            menu_dispose(&menu_function_numinfo_depslr, 0);
            menu_function_numinfo_depslr = NULL;

            menu_dispose(&menu_function_numinfo_deprange, 0);
            menu_function_numinfo_deprange = NULL;

            menu_dispose(&menu_function_numinfo_depname, 0);
            menu_function_numinfo_depname = NULL;
            }

        if(!mergebuf() || xf_inexpression || xf_inexpression_box || xf_inexpression_line || ev_doc_is_custom_sheet(current_docno()))
            {
            }
        else
            {
            P_SLOT tslot = travel_here();
            U8 type = (NULL != tslot) ? tslot->type : SL_TEXT;
            char coord[LIN_BUFSIZ];

            (void) write_ref(coord, elemof32(coord), current_docno(), curcol, currow); /* always current doc */

            switch(type)
                {
                case SL_NUMBER:
                    /* got number slot */
                    set_ev_slr(&menu_function_slr, curcol, currow);

                    menu_function_numinfo = menu_new_c(menu_function_numinfo_title, menu_function_numinfo_entries);

                    {
                    char  entry[LIN_BUFSIZ + 1];
                    char  valubuf[LIN_BUFSIZ + 1];

                    (void) expand_slot(current_docno(), tslot, currow, valubuf, LIN_BUFSIZ,
                                       DEFAULT_EXPAND_REFS /*expand_refs*/, TRUE /*expand_ats*/, TRUE /*expand_ctrl*/,
                                       FALSE /*allow_fonty_result*/, FALSE /*cff*/);

                    xstrkpy(entry, elemof32(entry), coord);
                    xstrkat(entry, elemof32(entry), " ");
                    xstrkat(entry, elemof32(entry), valubuf); /* plain non-fonty string */

                    substitute_spaces(entry, 0xA0);

                    /* create menu 'unparsed' cos entry may contain ',' */
                    menu_function_numinfo_slotvalue = menu_new_unparsed(menu_function_numinfo_value_title, entry);
                    }
                    menu_submenu (menu_function_numinfo, MENU_FUNCTION_NUMINFO_SLOTVALUE, menu_function_numinfo_slotvalue);
                    menu_setflags(menu_function_numinfo, MENU_FUNCTION_NUMINFO_SLOTVALUE, FALSE /*not ticked*/,
                                                                                 (NULL == menu_function_numinfo_slotvalue) /*greyed out*/);

                    create_sup_dep_submenu(MENU_FUNCTION_NUMINFO_SUPSLR     , &menu_function_numinfo_supslr,
                                           menu_function_numinfo_slr_title  , EV_DEPSUP_SLR  , FALSE, menu_function_numinfo);

                    create_sup_dep_submenu(MENU_FUNCTION_NUMINFO_SUPRANGE   , &menu_function_numinfo_suprange,
                                           menu_function_numinfo_range_title, EV_DEPSUP_RANGE, FALSE, menu_function_numinfo);

                    create_sup_dep_submenu(MENU_FUNCTION_NUMINFO_SUPNAME    , &menu_function_numinfo_supname,
                                           menu_function_numinfo_name_title , EV_DEPSUP_NAME , FALSE, menu_function_numinfo);

                    create_sup_dep_submenu(MENU_FUNCTION_NUMINFO_DEPSLR     , &menu_function_numinfo_depslr,
                                           menu_function_numinfo_slr_title  , EV_DEPSUP_SLR  , TRUE, menu_function_numinfo);

                    create_sup_dep_submenu(MENU_FUNCTION_NUMINFO_DEPRANGE   , &menu_function_numinfo_deprange,
                                           menu_function_numinfo_range_title, EV_DEPSUP_RANGE, TRUE, menu_function_numinfo);

                    create_sup_dep_submenu(MENU_FUNCTION_NUMINFO_DEPNAME    , &menu_function_numinfo_depname,
                                           menu_function_numinfo_name_title , EV_DEPSUP_NAME , TRUE, menu_function_numinfo);

                    break;

                case SL_TEXT:
                    {
                    BOOL can_compile_text = (NULL != tslot) ? (!is_protected_slot(tslot) && !isslotblank(tslot)) : FALSE;

                    set_ev_slr(&menu_function_slr, curcol, currow);

                    menu_function_textinfo =
                        menu_new_c(
                            (NULL != tslot)
                                ? menu_function_textinfo_title
                                : menu_function_textinfo_title_blank,
                            menu_function_textinfo_entries);

                    menu_setflags(menu_function_textinfo, MENU_FUNCTION_TEXTINFO_COMPILE, FALSE /*not ticked*/,
                                                                                 !can_compile_text /*greyed out*/);

                    create_sup_dep_submenu(MENU_FUNCTION_TEXTINFO_DEPSLR    , &menu_function_numinfo_depslr,
                                           menu_function_numinfo_slr_title  , EV_DEPSUP_SLR  , TRUE, menu_function_textinfo);

                    create_sup_dep_submenu(MENU_FUNCTION_TEXTINFO_DEPRANGE  , &menu_function_numinfo_deprange,
                                           menu_function_numinfo_range_title, EV_DEPSUP_RANGE, TRUE, menu_function_textinfo);

                    create_sup_dep_submenu(MENU_FUNCTION_TEXTINFO_DEPNAME   , &menu_function_numinfo_depname,
                                           menu_function_numinfo_name_title , EV_DEPSUP_NAME , TRUE, menu_function_textinfo);

                    break;
                    }
                }
            }

        if(menu_function_numinfo)
            {
            /* set menu entry to 'num. A1' */
            menu_submenu(menu_function, MENU_FUNCTION_SLOTINFO, menu_function_numinfo);
            menu_setflags(menu_function, MENU_FUNCTION_SLOTINFO, FALSE /*not ticked*/, FALSE /*not greyed*/);
            }
        else if(menu_function_textinfo)
            {
            /* set menu entry to 'text A1' */
            menu_submenu(menu_function, MENU_FUNCTION_SLOTINFO, menu_function_textinfo);
            menu_setflags(menu_function, MENU_FUNCTION_SLOTINFO, FALSE /*not ticked*/, FALSE /*not greyed*/);
            }
        else
            {
            /* just grey it out */
            menu_setflags(menu_function, MENU_FUNCTION_SLOTINFO, FALSE /*not ticked*/, TRUE /*greyed out*/);
            }
        }

    select_document_using_docno(old_docno);

    return(menu_function);
}

extern BOOL
function__event_menu_proc(
    void *handle,
    char *hit,
    BOOL submenurequest)
{
    DOCNO old_docno = current_docno();
    enum EV_RESOURCE_TYPES category = (enum EV_RESOURCE_TYPES) -1; /* invalid */

    IGNOREPARM(submenurequest);

    if(select_document_using_callback_handle(handle))
    {
        switch(*hit++)
        {
        case MENU_FUNCTION_DBASE:
            category = EV_RESO_DATABASE;
            break;

        case MENU_FUNCTION_DATE:
            category = EV_RESO_DATE;
            break;

        case MENU_FUNCTION_FINANCIAL:
            category = EV_RESO_FINANCE;
            break;

        case MENU_FUNCTION_LOOKUP:
            category = EV_RESO_LOOKUP;
            break;

        case MENU_FUNCTION_STRING:
            category = EV_RESO_STRING;
            break;

        case MENU_FUNCTION_COMPLEX:
            category = EV_RESO_COMPLEX;
            break;

        case MENU_FUNCTION_MATHS:
            category = EV_RESO_MATHS;
            break;

        case MENU_FUNCTION_MATRIX:
            category = EV_RESO_MATRIX;
            break;

        case MENU_FUNCTION_STAT:
            category = EV_RESO_STATS;
            break;

        case MENU_FUNCTION_TRIG:
            category = EV_RESO_TRIG;
            break;

        case MENU_FUNCTION_MISC:
            category = EV_RESO_MISC;
            break;

        case MENU_FUNCTION_CONTROL:
            category = EV_RESO_CONTROL;
            break;

        case MENU_FUNCTION_CUSTOM:
            category = EV_RESO_CUSTOM;
            break;

        case MENU_FUNCTION_NAMES:
            category = EV_RESO_NAMES;
            break;

        case MENU_FUNCTION_DEFINENAME:
            DefineName_fn();
            break;

        case MENU_FUNCTION_EDITNAME:
            EditName_fn(EV_RESO_NAMES, (*hit++)-1);     /* -1 because resource item numbers count from 0, */
            break;                                      /* whilst menu entries count from 1               */

        case MENU_FUNCTION_SLOTINFO:
            if(menu_function_numinfo)
                {
                switch(*hit++)
                    {
                    case MENU_FUNCTION_NUMINFO_SUPSLR:
                        goto_dep_or_sup_slot(&menu_function_slr, FALSE /* SUP */, EV_DEPSUP_SLR, (*hit++)-1);
                        break;

                    case MENU_FUNCTION_NUMINFO_SUPRANGE:
                        goto_dep_or_sup_slot(&menu_function_slr, FALSE /* SUP */, EV_DEPSUP_RANGE, (*hit++)-1);
                        break;

                    case MENU_FUNCTION_NUMINFO_SUPNAME:
                        goto_dep_or_sup_slot(&menu_function_slr, FALSE /* SUP */, EV_DEPSUP_NAME, (*hit++)-1);
                        break;

                    case MENU_FUNCTION_NUMINFO_DEPSLR:
                        goto_dep_or_sup_slot(&menu_function_slr, TRUE /* DEP */, EV_DEPSUP_SLR, (*hit++)-1);
                        break;

                    case MENU_FUNCTION_NUMINFO_DEPRANGE:
                        goto_dep_or_sup_slot(&menu_function_slr, TRUE /* DEP */, EV_DEPSUP_RANGE, (*hit++)-1);
                        break;

                    case MENU_FUNCTION_NUMINFO_DEPNAME:
                        goto_dep_or_sup_slot(&menu_function_slr, TRUE /* DEP */, EV_DEPSUP_NAME, (*hit++)-1);
                        break;
                    }
                }
            else if(menu_function_textinfo)
                {
                switch(*hit++)
                    {
                    case MENU_FUNCTION_TEXTINFO_COMPILE:
                        expedit_recompilecurrentslot_reporterrors();
                        break;

                    case MENU_FUNCTION_TEXTINFO_DEPSLR:
                        goto_dep_or_sup_slot(&menu_function_slr, TRUE /* DEP */, EV_DEPSUP_SLR, (*hit++)-1);
                        break;

                    case MENU_FUNCTION_TEXTINFO_DEPRANGE:
                        goto_dep_or_sup_slot(&menu_function_slr, TRUE /* DEP */, EV_DEPSUP_RANGE, (*hit++)-1);
                        break;

                    case MENU_FUNCTION_TEXTINFO_DEPNAME:
                        goto_dep_or_sup_slot(&menu_function_slr, TRUE /* DEP */, EV_DEPSUP_NAME, (*hit++)-1);
                        break;
                    }
                }
            else
                {
                }

            break;
        }
    }

    if((S32) category >= 0)
        PasteName_fn(category, (*hit++)-1);             /* -1 because resource item numbers count from 0, */
                                                        /* whilst menu entries count from 1               */

    select_document_using_docno(old_docno);

    return(TRUE);
}

static void
goto_dep_or_sup_slot(
    _InRef_     PC_EV_SLR slrp,
    S32 get_deps,
    _InVal_     enum EV_DEPSUP_TYPES category,
    S32 itemno)
{
    EV_DOCNO    cur_docno;
    EV_OPTBLOCK optblock;
    EV_DEPSUP   depsup;
    EV_DATA     evdata;

    trace_0(TRACE_APP_EXPEDIT, "goto_dep_or_sup_slot");

    cur_docno = (EV_DOCNO) current_docno();

    ev_set_options(&optblock, cur_docno);

    ev_enum_dep_sup_init(&depsup, slrp, category, get_deps);

    if(ev_enum_dep_sup_get(&depsup, &itemno, &evdata) >= 0)
        {
    /*    ev_decode_data(namebuf, curdoc, &evdata, &optblock);*/

        if(evdata.did_num == RPN_DAT_SLR)
            {
            trace_3(TRACE_APP_EXPEDIT, "goto - docno,col,row (%u,%d,%d)",
                                      evdata.arg.slr.docno,
                                      (int) evdata.arg.slr.col,
                                      (int) evdata.arg.slr.row);

            /* based on GotoSlot_fn() */
            if(mergebuf())
                {
                select_document_using_docno(evdata.arg.slr.docno);
                xf_frontmainwindow = TRUE;
                chknlr((COL) evdata.arg.slr.col, (ROW) evdata.arg.slr.row);
                lecpos = lescrl = 0;
                xf_caretreposition = TRUE;
                draw_screen();
                draw_caret();
                }
            }
        }
}

/******************************************************************************
*
*             dynamically create menu structures for main window
*
******************************************************************************/

/*
main menu
*/

static menu main_menu = NULL;

/*
offsets starting from 1
*/

#define mo_main_dynamic 1

/******************************************************************************
*
* create submenu for headline[i]
*
******************************************************************************/

static BOOL
createsubmenu(
    MENU_HEAD * mhptr,
    BOOL short_m)
{
    menu m = NULL;
    MENU *mptr       = mhptr->tail;
    MENU *last_mptr  = mptr + mhptr->items;
    char description[256];
    char sep = '\0';

    trace_1(TRACE_APP_PD4, "createsubmenu(short = %s)", trace_boolstring(short_m));

    do  {
        char *ptr    = &description[4];    /* could be 2 but 4 saves code */
        /* process true if displaying long menus or item on short menus */
        BOOL process = !short_m  ||  on_short_menus(mptr->flags);

        if(!mptr->title)
            {
            trace_2(TRACE_APP_PD4, "looking at item line: short = %s -> process = %s",
                    trace_boolstring(!(mptr->flags & LONG_ONLY)),
                    trace_boolstring(process));

            if(process && m /* !sep */)
                sep = '|';
            }
        else
            {
            trace_3(TRACE_APP_PD4, "looking at command %s: short = %s -> process = %s",
                    trace_string(*mptr->title),
                    trace_boolstring(!(mptr->flags & LONG_ONLY)),
                    trace_boolstring(process));

            if(process)
                {
                if(get_menu_item(mhptr, mptr, ptr))
                    /* menu item has a submenu/dbox */
                    *--ptr = '>';

                if(!m /* sep == '\0' */)
                    {
                    /* first item */
                    m = menu_new_c(*mhptr->name, ptr);
                    if(!m)
                        return(FALSE);
                    }
                else
                    {
                    *--ptr = sep;
                    menu_extend(m, ptr);
                    }

                sep = ',';
                }
            }
        }
    while(++mptr < last_mptr);

    mhptr->m = m;

    return(TRUE);
}

/******************************************************************************
*
* encode submenu for headline[i]
*
******************************************************************************/

static void
encodesubmenu(
    MENU_HEAD * mhptr,
    BOOL short_m)
{
    MENU *mptr       = mhptr->tail;
    MENU *last_mptr  = mptr + mhptr->items;
    S32 offset      = 1;
    BOOL tick, fade;
    S32 flag;
    BOOL has_a_document = (NO_DOCUMENT != find_document_with_input_focus());

    trace_1(TRACE_APP_PD4, "encodesubmenu(%s)", trace_boolstring(short_m));

    if(mhptr->m)
    do  {
        if(mptr->title)    /* ignore gaps */
            {
            flag = mptr->flags;

            /* if this is a long menu, or item on short menu */
            if(!short_m  ||  on_short_menus(flag))
                {
                if(flag & (TICKABLE | GREYABLE | GREY_EXPEDIT | DOC_REQ))
                    {
                    tick = fade = FALSE;

                    if(flag & TICKABLE)
                        tick = flag & TICK_STATUS;

                    if(flag & GREYABLE)
                        fade = flag & GREY_STATUS;

                    if(flag & GREY_EXPEDIT)
                        fade = fade || (xf_inexpression || xf_inexpression_box || xf_inexpression_line);

                    if(flag & DOC_REQ)
                        fade = fade || !has_a_document;

                    menu_setflags(mhptr->m, offset, tick, fade);
                    }

                ++offset;
                }
            }
        }
    while(++mptr < last_mptr);
}

/******************************************************************************
*
*  Create main menu once at startup
* and whenever long/short form changed
*
******************************************************************************/

static void
createmainmenu(
    BOOL short_m)
{
    char description[256];
    S32 head_i;
    char *ptr;
    BOOL needsmain = (main_menu == NULL);

    trace_1(TRACE_APP_PD4, "createmainmenu(%s)", trace_boolstring(short_m));

    /* iconbar menu bits */

    /* dispose of existing submenu at this point */
    menu_dispose((menu *) &iconbar_headline[0].m, 0);
    iconbar_headline[0].m = NULL;

    if(!createsubmenu(&iconbar_headline[0], short_m))
        reperr_null(status_nomem());

    if(iconbar_headline[0].m)
        (void) menu_submenu(iconbar_menu, mo_iconbar_choices, iconbar_headline[0].m);

#if FALSE
    /* function menu bits */

    /* dispose of existing submenu at this point */
    menu_dispose((menu *) &iconbar_headline[1].m, 0);
    iconbar_headline[1].m = NULL;

    if(!createsubmenu(&iconbar_headline[1], short_m))
        reperr_null(status_nomem());

    if(iconbar_headline[1].m)
        (void) menu_submenu(menu_function, MENU_FUNCTION_NAMES, iconbar_headline[1].m);
#endif

    /* create dynamic main menu bits */

    for(head_i = 0; head_i < head_size; head_i++)
        if(headline[head_i].installed)
            {
            /* create main menu if needed - once only operation */
            if(needsmain)
                {
                ptr = description;
                *++ptr = '>';
                strcpy(ptr + 1, *headline[head_i].name);

                /* create/extend the top level menu */
                if(!main_menu)
                    main_menu = menu_new_c(product_id(), ptr);
                else
                    {
                    *--ptr = ',';
                    menu_extend(main_menu, ptr);
                    }

                if(!main_menu)
                    break;
                }

            /* dispose of existing submenu at this point */
            menu_dispose((menu *) &headline[head_i].m, 0);
            headline[head_i].m = NULL;

            /* create the submenu and attach it here */

            /* complain a little */
            if(!createsubmenu(&headline[head_i], short_m))
                reperr_null(status_nomem());

            (void) menu_submenu(main_menu, mo_main_dynamic + head_i, headline[head_i].m);
            }
}

/******************************************************************************
*
* encode main menu each MENU press
*
******************************************************************************/

static void
encodemainmenu(
    BOOL short_m)
{
    S32 head_i;

    /* encode main menu */

    for(head_i = 0; head_i < head_size; head_i++)
        if(headline[head_i].installed)
            encodesubmenu(&headline[head_i], short_m);

    /* encode chart submenus */
    pdchart_submenu_maker();
}

/******************************************************************************
*
*  process main window MENU press
*
******************************************************************************/

static menu
main_menu_maker(
    void * handle)
{
    trace_1(TRACE_APP_PD4, "main_menu_maker(%p)", handle);

    /* ensure font selector goes bye-bye */
    pdfontselect_finalise(TRUE);

    if(!select_document_using_callback_handle(handle))
        return(NULL);

    xf_caretreposition = TRUE;
    draw_caret();

    /* do menu encoding */

    encodemainmenu(short_menus());

    riscmenu__was_main = FALSE;

    trace_1(TRACE_APP_PD4, "main menus encoded: returning menu &%p", report_ptr_cast(main_menu));

    return(main_menu);
}

static BOOL
insertfontsize(
    F64 size)
{
    char array[32];

    (void) xsnprintf(array, elemof32(array), ",%.3g", size / 16.0);

    return(insert_string(array, FALSE));
}

static struct PDFONTSELECT
{
    DOCNO docno;
    S32       setting;
}
pdfontselect;

_Check_return_
extern BOOL
pdfontselect_is_active(void)
{
    return(DOCNO_NONE != pdfontselect.docno);
}

static F64
get_leading_from_selector(
    wimp_w window)
{
    F64 dleading;
    wimp_icon icon;
    char *icon_string;

    wimp_get_icon_info(window, FONT_LEADING, &icon);

    icon_string = icon.data.indirecttext.buffer;

    if(sscanf(icon_string, "%lf", &dleading) < 1)
        return(0.);

    return(dleading);
}

static void
new_font_leading_based_on_field(
    wimp_w window)
{
    F64 dleading = get_leading_from_selector(window);

    if(dleading == 0)
        {
        auto_line_height = TRUE;
        new_font_leading_based_on_y();
        }
    else
        {
        S32 leading = (S32) (dleading * 1000.0);
        new_font_leading(leading);
        }
}

static void
pdfontselect_try_me(
    const char * font_name,
    PC_F64 width,
    PC_F64 height,
    wimp_w w)
{
    if(pdfontselect.docno != DOCNO_NONE)
        {
        DOCNO old_docno = change_document_using_docno(pdfontselect.docno);
        S32 font_x = (S32) (*width  * 16.0);
        S32 font_y = (S32) (*height * 16.0);

        /* printer font selection; may unset riscos_fonts on error in repaint */
        if(font_name && (0 != _stricmp(font_name, "System")))
            {
            if(pdfontselect.setting)
                {
                /* set new font */
                (void) mystr_set(&global_font, font_name);
                global_font_x = font_x;
                global_font_y = font_y;
                riscos_fonts = TRUE;

                /* bump up the leading (and reduce page length) if needed */
                if(global_font_y * 1000 > global_font_leading_mp * 16)
                    auto_line_height = TRUE;

                if(auto_line_height)
                    new_font_leading_based_on_y();
                else
                    new_font_leading_based_on_field(w);

                {
                F64 dleading = global_font_leading_mp / 1000.0;

                win_setonoff(w, FONT_LEADING_AUTO, auto_line_height);
                win_setdouble(w, FONT_LEADING, &dleading, 2);
                }

                }
            else
                {
                /* insert font reference */
                filbuf();

                if(NULLCH != get_text_at_char())
                    {
                    if( insert_string(d_options_TA, FALSE)  &&
                        insert_string("F:", FALSE)          &&
                        insert_string(font_name, FALSE)     )
                        {
                        if( (font_x != global_font_x)  ||
                            (font_y != global_font_y)  )
                            {
                            insertfontsize(font_x);

                            if(font_x != font_y)
                                insertfontsize(font_y);
                            }

                        insert_string(d_options_TA, FALSE);
                        }
                    }
                }
            }
        else
            {
            if(pdfontselect.setting)
                {
                str_clr(&global_font);
                riscos_fonts = FALSE;

                /* get height information recomputed */
                new_font_leading(global_font_leading_mp);
                }
            else
                /* can't insert a ref to standard system font! */ ;
            }

        if(pdfontselect.setting)
            {
            riscos_font_error = FALSE;
            xf_draweverything = TRUE;
            xf_caretreposition = TRUE;
            filealtered(TRUE);
            draw_screen();
            /*draw_caret();*/
            }
        else
            {
            out_currslot = TRUE;
            filealtered(TRUE);
            draw_screen();
            }

        select_document_using_docno(old_docno);
        }
}

/* ensure font selector goes bye-bye */

extern void
pdfontselect_finalise(
    BOOL kill)
{
    if(!kill)
        kill = (pdfontselect.docno == current_docno());

    if(kill && (pdfontselect.docno != DOCNO_NONE))
        {
        pdfontselect.docno = DOCNO_NONE;
        /* this will cause process to end and return */
        fontselect_closewindows();
        }
}

/*
change the line height in the font selector
bumping up/down increments/decrements it
clicking Auto sets it to 1.2 times font height
*/

static void
bump_line_height(
    wimp_w window,
    BOOL up)
{
    F64 dleading;
    S32 leading;
    char array[32];
    BOOL is_int;

    dleading = get_leading_from_selector(window);

    /* round down */
    leading = (S32) dleading;

    is_int = ((F64) leading) == dleading;

    if(up)
        leading++;
    else if(is_int)
        leading--;

    (void) sprintf(array, "%d", (leading <= 0) ? 1 : leading);

    set_icon_text(window, FONT_LEADING, array);
}

static void
show_new_auto_line_height(
    wimp_w w,
    PC_F64 height)
{
    F64 dleading = *height * 1.2;

    win_setdouble(w, FONT_LEADING, &dleading, 2);
}

static void
pdfontselect_init_fn(
    wimp_w fontselect_wind,
    P_ANY init_handle)
{
    IGNOREPARM(init_handle);

    if(pdfontselect.docno != DOCNO_NONE)
        {
        F64 dleading = global_font_leading_mp / 1000.0;
        DOCNO old_docno = change_document_using_docno(pdfontselect.docno);

        win_unfadefield(fontselect_wind, FONT_LEADING);
        win_unfadefield(fontselect_wind, FONT_LEADING_DOWN);
        win_unfadefield(fontselect_wind, FONT_LEADING_UP);
        win_unfadefield(fontselect_wind, FONT_LEADING_AUTO);

        win_setonoff(fontselect_wind, FONT_LEADING_AUTO, auto_line_height);

        win_setdouble(fontselect_wind, FONT_LEADING, &dleading, 2);

        select_document_using_docno(old_docno);
        }
}

static BOOL
pdfontselect_unknown_fn(
    const char * font_name,
    PC_F64 width,
    PC_F64 height,
    const wimp_eventstr * e,
    P_ANY try_handle,
    BOOL try_anyway)
{
    BOOL processed = FALSE;

    IGNOREPARM(try_handle);

    if(pdfontselect.docno != DOCNO_NONE)
        {
        DOCNO old_docno = change_document_using_docno(pdfontselect.docno);

        switch(e->e)
            {
            case wimp_EBUT:
                switch(e->data.but.m.i)
                    {
                    /* can put in click on line height stuff here */
                    case FONT_LEADING_UP:
                    case FONT_LEADING_DOWN:
                        {
                        dbox_field f;
                        auto_line_height = FALSE;
                        f = (dbox_field) e->data.but.m.i; /* SKS 22.10.91 made bumpers reverse in RISC OS fashion */
                        dbox_adjusthit(&f, FONT_LEADING_UP, FONT_LEADING_DOWN, riscos_adjustclicked());
                        bump_line_height(e->data.but.m.w, (f == FONT_LEADING_UP));
                        win_setonoff(e->data.but.m.w, FONT_LEADING_AUTO, FALSE /*off*/);
                        }
                        break;

                    case FONT_LEADING_AUTO:
                        auto_line_height = win_getonoff(e->data.but.m.w, FONT_LEADING_AUTO);
                        if(auto_line_height)
                            show_new_auto_line_height(e->data.but.m.w, height);
                        break;

                    default:
                        if(try_anyway)
                            pdfontselect_try_me(font_name, width, height, e->data.but.m.w);
                        else if(auto_line_height)
                            show_new_auto_line_height(e->data.but.m.w, height);
                        break;
                    }
                break;

            case wimp_EKEY:
                switch(e->data.key.chcode)
                    {
                    default:
                        if(try_anyway)
                            pdfontselect_try_me(font_name, width, height, e->data.key.c.w);
                        break;
                    }
                break;

            default:
                break;
            }

        select_document_using_docno(old_docno);
        }

    return(processed);
}

extern void
PrinterFont_fn(void)
{
    F64 width, height;

    if(riscdialog_in_dialog())
        {
        riscdialog_front_dialog();
        reperr_null(ERR_ALREADY_IN_DIALOG);
        return;
        }

    #if 0
    /* SKS 25oct96 refresh list every time to stop punters wingeing as much */
    fontlist_free_font_tree(font__tree);
    #endif

    if(!fontselect_prepare_process())
        return;

    riscmenu_clearmenutree();

    pdfontselect.setting = 1;

    pdfontselect.docno = current_docno();

    width  = global_font_x / 16.0;
    height = global_font_y / 16.0;

    fontselect_process(Printer_font_STR,                         /* win title */
                       fontselect_SETFONT | fontselect_SETTITLE, /* flags */
                       global_font,                              /* font_name */
                       &width,                                   /* font width */
                       &height,                                  /* font height */
                       pdfontselect_init_fn, NULL,
                       pdfontselect_unknown_fn, NULL);

    /* be careful here, there might not be a current document anymore! */

    pdfontselect.docno = DOCNO_NONE;
}

extern void
InsertFont_fn(void)
{
    F64 width, height;

    if(riscdialog_in_dialog())
        {
        riscdialog_front_dialog();
        reperr_null(ERR_ALREADY_IN_DIALOG);
        return;
        }

    /* rjm adds this on 5.10.91.  Is it right? Does it need error message?, RCM & SKS 27.11.91 yes it does */
    if(!riscos_fonts)
        {
        reperr_null(create_error(ERR_SYSTEMFONTSELECTED));
        return;
        }

    if(!fontselect_prepare_process())
        return;

    riscmenu_clearmenutree();

    pdfontselect.setting = 0;

    pdfontselect.docno = current_docno();

    width  = global_font_x / 16.0;
    height = global_font_y / 16.0;

    fontselect_process(Insert_font_STR,                          /* win title */
                       fontselect_SETFONT | fontselect_SETTITLE, /* flags */
                       global_font,                              /* font_name */
                       &width,                                   /* font width */
                       &height,                                  /* font height */
                       NULL, NULL,
                       pdfontselect_unknown_fn, NULL);

    /* be careful here, there might not be a current document anymore! */

    pdfontselect.docno = DOCNO_NONE;
}

/******************************************************************************
*
* process main window menu selection
* caused by click on submenu entry
* or wandering off the right => of one
*
******************************************************************************/

static BOOL
decodesubmenu(
    const MENU_HEAD * mhptr,
    S32 offset,
    S32 nextoffset,
    BOOL submenurequest)
{
    BOOL short_m = short_menus();
    const MENU * mptr   = mhptr->tail;
    S32 i = 1;    /* NB. offset == 1 is start of array */
    S32 funcnum;
    BOOL processed = TRUE;

    trace_2(TRACE_APP_PD4, "decodesubmenu([%d][%d])", offset, nextoffset);

    for(; ; mptr++)
        {
        #if TRACE_ALLOWED
        PC_U8 command = trace_string(*mptr->title);
        #endif

        /* don't count the gaps and the missing items --- NB. NOT *mptr->title! */
        if(!mptr->title)
            continue;

        if(short_m  && !on_short_menus(mptr->flags))
            {
            trace_1(TRACE_APP_PD4, "--- ignoring command %s", command);
            continue;
            }

        trace_2(TRACE_APP_PD4, "--- looking at command %s, i = %d", command, i);

        if(i == offset)
            {
            trace_1(TRACE_APP_PD4, "--- found command %s", command);
            break;
            }

        ++i;
        }

    funcnum = mptr->funcnum;

    /* look for direct hits on menus as opposed to submenu opening requests */

    switch(funcnum)
        {
        case N_ChartSelect:
        case N_ChartDelete:
        case N_ChartEdit:
        case N_ChartAdd:
        case N_ChartRemove:
            if(submenurequest)
                {
                /* a change from the usual way of things */
                pdchart_submenu_show(submenurequest);
                break;
                }

            pdchart_submenu_select_from(nextoffset);

            if(funcnum == N_ChartSelect)
                /* function performed */
                break;

            application_process_command(funcnum);
            break;

        case N_SaveChoices:
            {
            /* can only save choices from a specified document as it takes structure */
            P_DOCU p_docu;

            if(NO_DOCUMENT != (p_docu = find_document_with_input_focus()))
                {
                select_document(p_docu);
                application_process_command(funcnum);
                }

            break;
            }

        case N_SaveFile:
            if(!submenurequest)
                funcnum = N_SaveFileAsIs;

            /* deliberate drop thru */

        default:
            /* call the appropriate function */
            application_process_command(funcnum);
            break;
        }

    return(processed);
}

/******************************************************************************
*
* process main window menu selection
* caused either by click on menu entry
* or menu warning when leaving =>
*
******************************************************************************/

static BOOL
main_menu_handler(
    void * handle,
    char * hit,
    BOOL submenurequest)
{
    S32 selection    = hit[0];
    S32 subselection = hit[1];
    BOOL processed   = TRUE;

    trace_3(TRACE_APP_PD4, "main_menu_handler([%d][%d]): submenurequest = %s",
            selection, subselection, trace_boolstring(submenurequest));

    if(select_document_using_callback_handle(handle))
    {
        switch(selection)
        {
        case mo_noselection: /* I don't believe this ever happens */
            break;           /* as it'd be far too useful ... */

        default:
            /* an event to do with the dynamic menu bits */
            switch(subselection)
                {
                case mo_noselection:
                    /* open the given submenu */
                    processed = FALSE;
                    break;

                default:
                    /* always decode the selection: the called function
                     * may put up a dialog box, in which case dbox does
                     * the right thing.
                    */
                    {
                    /* lookup in the dynamic menu structure */
                    int offset  = selection - mo_main_dynamic;
                    int thisoff = 0;
                    int head_i;

                    for(head_i = 0; head_i < head_size; head_i++)
                        if(headline[head_i].installed)
                            if(offset == thisoff++)
                                {
                                processed = decodesubmenu(&headline[head_i],
                                                          subselection,
                                                          hit[2],
                                                          submenurequest);
                                break;
                                }
                    }
                    break;
                }
            break;
        }
    }

    return(processed);
}

/******************************************************************************
*
*  attach menu tree to this window
*
******************************************************************************/

extern void
riscmenu_attachmenutree(
    wimp_w w)
{
    event_attachmenumaker_to_w_i(
        w, (wimp_i) -1,
        main_menu_maker,
        main_menu_handler,
        (void *)(uintptr_t)current_docno());
}

/******************************************************************************
*
* (re)build the menu tree in short or long form
*
******************************************************************************/

extern void
riscmenu_buildmenutree(
    BOOL short_m)
{
    createmainmenu(short_m);
}

/******************************************************************************
*
*  clear menu tree
*
******************************************************************************/

extern void
riscmenu_clearmenutree(void)
{
    trace_0(TRACE_APP_PD4, "destroying menu tree");

    event_clear_current_menu();
}

/******************************************************************************
*
* detach menu tree from this window
*
******************************************************************************/

extern void
riscmenu_detachmenutree(
    wimp_w w)
{
    event_attachmenumaker_to_w_i(w, (wimp_i) -1, NULL, NULL, NULL);
}

extern void
riscmenu_initialise_once(void)
{
    /* create menu for icon bar icons */

    iconbar_menu = menu_new_c(product_ui_id(), iconbar_menu_entries);
    function__event_menu_maker();       /* setup menu_function */

    if(!iconbar_menu)
        reperr_fatal(reperr_getstr(status_nomem()));

#if TRACE_ALLOWED
    if(trace_is_enabled())
        menu_extend(iconbar_menu, de_const_cast(char *, iconbar_menu_entries_trace));
#endif

    event_attachmenumaker_to_w_i(
        win_ICONBAR, (wimp_i) -1,
        iconbar_menu_maker,
        iconbar_menu_handler,
        NULL);

    /* main menu created by menu_state_changed() soon after */
}

extern void
riscmenu_tidy_up(void)
{
    /* free 'documents' menu */
    menu_dispose(&iw_menu, 0);
    iw_menu = NULL;
}

/* end of riscmenu.c */
