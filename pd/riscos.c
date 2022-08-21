/* riscos.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* RISC OS specifics for PipeDream */

/* Stuart K. Swales 24 Jan 1989 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

#ifndef __cs_event_h
#include "cs-event.h"
#endif

#ifndef __menu_h
#include "menu.h"
#endif

#ifndef __cs_dbox_h
#include "cs-dbox.h"
#endif

#ifndef __akbd_h
#include "akbd.h"
#endif

#ifndef __cs_resspr_h
#include "cs-resspr.h"
#endif

#ifndef __cs_template_h
#include "cs-template.h"
#endif

/* external header files */
#include "riscos_x.h"
#include "riscdraw.h"
#include "pd_x.h"
#include "ridialog.h"
#include "riscmenu.h"
#include "colh_x.h"

#ifndef __cs_xferrecv_h
#include "cs-xferrecv.h"
#endif

#include "cmodules/typepack.h"

#include "cmodules/riscos/wlalloc.h"

#include "cmodules/riscos/osfile.h"

#ifdef PROFILING
#include "cmodules/riscos/profile.h"
#endif

/*
internal functions
*/

static BOOL
default_event_handler(
    wimp_eventstr *e,
    void *handle);

static void
send_end_markers(void);

static BOOL
draw_insert_filename(
    PC_U8 filename);

#define TRACE_DRAW      (TRACE_APP_PD4 & 0)
#define TRACE_NULL      (TRACE_APP_PD4)

static BOOL g_kill_duplicates = FALSE;

/* ------------------------ file import/export --------------------------- */

/* ask for scrap file load: all errors have been reported locally */

static void
scraptransfer_file_PDChart(
    _InVal_     BOOL iconbar)
{
    char grname[BUF_MAX_PATHSTRING];
    char myname[BUF_MAX_PATHSTRING];

    /* check with chart editor to see if he's saving to us - if so invent
     * relative filename using leafname supplied by chart editor
    */
    if(gr_chart_saving_chart(grname))
    {
        if(file_is_rooted(grname))
        {
            /* just insert a ref if it is saved */
            draw_insert_filename(grname);
            return;
        }

        /* fault these as we can't store them anywhere safe relative
         * to the iconbar or relative to this unsaved file
        */
        if(iconbar)
            reperr_null(GR_CHART_ERR_DRAWFILES_MUST_BE_SAVED);
        else if(!file_is_rooted(currentfilename()))
            reperr_null(ERR_CHARTIMPORTNEEDSSAVEDDOC);
        else
        {
            U32 prefix_len;
            S32 num = 0;

            file_get_prefix(myname, elemof32(myname), currentfilename());

            prefix_len = strlen32(myname);

            do  {
                if(++num > 99) /* only two digits in printf string */
                    break;

                /* invent a new leafname and stick it on the end */
                (void) xsnprintf(myname + prefix_len, elemof32(myname) - prefix_len,
                        string_lookup(GR_CHART_MSG_DEFAULT_CHARTINNAMEZD),
                        file_leafname(currentfilename()),
                        num);

                /* see if there's any file of this name already */
            }
            while(file_is_file(myname));

            xferrecv_import_via_file(myname);
        }
    }
    else
    {
        xferrecv_import_via_file(NULL); /* can't manage RAM load */
    }
}

static void
scraptransfer_file_PipeDream(void)
{
    xferrecv_import_via_file(NULL); /* can't manage RAM load */
}

static void
scraptransfer_file_others(
    _InVal_       FILETYPE_RISC_OS filetype)
{
    if(image_cache_can_import(filetype))
        /* fault these as we can't store them anywhere safe */
        reperr_null(GR_CHART_ERR_DRAWFILES_MUST_BE_SAVED);
    else
        xferrecv_import_via_file(NULL); /* can't manage ram load */
}

static void
scraptransfer_file(
    const WimpDataSaveMessage * const data_save,
    _InVal_       BOOL iconbar)
{
    S32 size;
    const FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkimport(&size);

    switch(filetype)
    {
    case FILETYPE_DIRECTORY:
    case FILETYPE_APPLICATION:
        reperr(FILE_ERR_ISADIR, data_save->leaf_name);
        break;

    case FILETYPE_PD_CHART:
        scraptransfer_file_PDChart(iconbar);
        break;

    case FILETYPE_PIPEDREAM:
        scraptransfer_file_PipeDream();
        break;

    default:
        scraptransfer_file_others(filetype);
        break;
    }
}

/* now filename is guaranteed to be readable (iff filetype_option > 0) */

static BOOL
riscos_LoadFile(
    _In_z_      PCTSTR filename,
    BOOL inserting,
    STATUS filetype_option)
{
    BOOL temp_file = !xferrecv_file_is_safe();
    BOOL res;
    BOOL recording;
    LOAD_FILE_OPTIONS load_file_options;

    /* doing it here keeps reporting consistent for dragged and double-clicked files */
    if(filetype_option <= 0)
    {
        STATUS status;
        if(0 == filetype_option)
            status = create_error(ERR_NOTFOUND);
        else
            status = filetype_option;
        reperr(status, filename);
        return(FALSE);
    }

    reportf("riscos_LoadFile been asked to %s file %u:'%s', temp = %s; DOCNO = %d",
            inserting ? "insert" : "load", strlen32(filename), filename,
            report_boolstring(temp_file), current_docno());

    /* set up d_load[] for macro recorder */

    res = init_dialog_box(D_LOAD);

    if(res)
        res = mystr_set(&d_load[0].textfield, filename);

    if(res)
    {
        if(inserting)
            d_load[1].option = 'Y'; /* Insert */

        /* loadfile will now create its own document if not inserting */

        d_load[3].option = (char) filetype_option; /* filetype */

        /* spurt pseudo-load-command to macro file */
        recording = macro_recorder_on && !temp_file;

        if(recording)
        {
            DHEADER * dhptr = &dialog_head[D_LOAD];

            out_comm_start_to_macro_file(N_LoadFile);
            out_comm_parms_to_macro_file(dhptr->dialog_box, dhptr->items, FALSE);
        }

        load_file_options_init(&load_file_options, filename, filetype_option);
        load_file_options.temp_file = temp_file;
        load_file_options.inserting = inserting;
        res = loadfile(filename, &load_file_options);
        trace_1(TRACE_APP_PD4, "loadfile returned %d", res);

        if(recording)
            out_comm_end_to_macro_file(N_LoadFile);
    }

    if(res && is_current_document())
        draw_screen();

    return(res);
}

extern void
print_file(
    _In_z_      PCTSTR filename)
{
    BOOL ok = 0;
    STATUS filetype_option;

    reportf("print_file(%u:%s)", strlen32(filename), filename);

    filetype_option = find_filetype_option(filename, FILETYPE_UNDETERMINED);

    /* yes, we can print these! (sort of) */
    if(PD4_CHART_CHAR == filetype_option)
    {
        if(create_new_untitled_document())
        {
            ok = draw_insert_filename(filename);

            if(!ok)
                destroy_current_document();
        }
    }
    else
        ok = riscos_LoadFile(filename, FALSE, filetype_option /*may be 0,Err*/); /* load as new file */

    if(ok)
    {
        print_document();

        destroy_current_document();
    }
}

/* ----------------------------------------------------------------------- */

extern BOOL
riscos_quit_okayed(
    S32 nmodified)
{
    DOCNO old_docno = current_docno();
    P_DOCU p_docu;
    enum RISCDIALOG_QUERY_SDC_REPLY SDC_res;
    char query_quit_statement[256];

    dbox_note_position_on_completion(TRUE);

    consume_int(xsnprintf(query_quit_statement, elemof32(query_quit_statement),
                          (nmodified == 1)
                              ? Zd_file_edited_in_Zs_are_you_sure_STR
                              : Zd_files_edited_in_Zs_are_you_sure_STR,
                          nmodified,
                          product_ui_id()));

    SDC_res = riscdialog_query_quit_SDC(query_quit_statement, query_quit_SDC_Q_STR);

    /* Discard -> ok, quit application */
    /* Cancel  -> abandon closedown */
    /* Save    -> save files, then quit application */

    if(riscdialog_query_SDC_SAVE == SDC_res)
    {
        /* loop over all documents in sequence trying to save them */

        for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
        {
            select_document(p_docu);

            dbox_note_position_on_completion(TRUE);

            /* If punter (or program) cancels any save, he means abort the shutdown */
            if(riscdialog_query_SDC_CANCEL == riscdialog_query_save_or_discard_existing())
            {
                SDC_res = riscdialog_query_SDC_CANCEL;
                break;
            }
        }
    }

    dbox_noted_position_trash();

    /* if not aborted then all modified documents either saved or ignored - delete all documents (must be at least one) */

    switch(SDC_res)
    {
    case riscdialog_query_SDC_SAVE:
    case riscdialog_query_SDC_DISCARD:
        {
        for(;;)
        {
            if(NO_DOCUMENT == (p_docu = first_document()))
                break;

            select_document(p_docu);

            destroy_current_document();
        }

        select_document(NO_DOCUMENT);
        return(1);
        }

    default:
        select_document_using_docno(old_docno);
        return(0);
    }
}

/* ----------------------------------------------------------------------- */

typedef S32 riscos_icon;

/* has this module been initialised? (shared global) */

static BOOL riscos__initialised = FALSE;

/******************************************************************************
*
* main window (shared globals)
*
******************************************************************************/

static const char  rear_window_name[] = "rear_wind";
static const char  main_window_name[] = "main_wind";
static const char  colh_window_name[] = "colh_wind";

static wimp_wind * main_window_definition;        /* ptr to actual window def NB. this might not be word aligned! */
static S32         main_window_initial_y1;
static S32         main_window_default_height;
#define            main_window_y_bump           (wimptx_title_height() + 16)

/*
can we print a file of this filetype?
*/

static BOOL
pd_can_print(
    _InVal_     FILETYPE_RISC_OS rft)
{
    switch(rft)
    {
    case FILETYPE_PD_CHART:
    case FILETYPE_PIPEDREAM:
        return(TRUE);

    default:
        return(FALSE);
    }
}

/*
can we 'run' a file of this filetype?
*/

static BOOL
pd_can_run(
    _InVal_     FILETYPE_RISC_OS rft)
{
    switch(rft)
    {
    case FILETYPE_PD_CHART:
    case FILETYPE_PD_MACRO:
    case FILETYPE_PIPEDREAM:
        return(TRUE);

    default:
        return(FALSE);
    }
}

/******************************************************************************
*
*                           icon bar processing
*
******************************************************************************/

/******************************************************************************
*
* process button click event for icon bar
*
******************************************************************************/

static BOOL
iconbar_event_Mouse_Click_Button_Select(void)
{
    if(host_ctrl_pressed())
    { /*EMPTY*/ } /* reserved */
    else if(host_shift_pressed())
    {
        TCHARZ filename[BUF_MAX_PATHSTRING];

        /* open the user's Choices directory display, from where they can find CmdFiles, Templates etc. */
        tstr_xstrkpy(filename, elemof32(filename), TEXT("<Choices$Write>") FILE_DIR_SEP_STR TEXT("PipeDream") FILE_DIR_SEP_STR);
        tstr_xstrkat(filename, elemof32(filename), TEXT("X"));

        filer_opendir(filename);
    }
    else
        application_process_command(N_LoadTemplate);

    return(TRUE);
}

static BOOL
iconbar_event_Mouse_Click_Button_Adjust(void)
{
    if(host_ctrl_pressed())
    { /*EMPTY*/ } /* reserved */
    else if(host_shift_pressed())
    { /*EMPTY*/ } /* reserved */
    else
        application_process_command(N_NewWindow);

    return(TRUE);
}

static BOOL
iconbar_event_Mouse_Click(
    const WimpMouseClickEvent * const mouse_click)
{
    if(mouse_click->buttons == Wimp_MouseButtonSelect)
        return(iconbar_event_Mouse_Click_Button_Select());

    if(mouse_click->buttons == Wimp_MouseButtonAdjust)
        return(iconbar_event_Mouse_Click_Button_Adjust());

    trace_0(TRACE_APP_PD4, "wierd mouse button click - ignored");
    return(FALSE);
}

/* create an icon on the icon bar */

static int iconbar_icon_handle;

static char iconbar_spritename[12 + 1];

static void
iconbar_new_sprite_setup(
    _Out_ WimpIconBlockWithBitset * p_icon)
{
    zero_struct_ptr_fn(p_icon);

    p_icon->bbox.xmin = 0;
    p_icon->bbox.ymin = 0;
    p_icon->bbox.xmax = 68 /* sprite width */;
    p_icon->bbox.ymax = 68 /* sprite height */;

    p_icon->flags.bits.sprite = 1;
    p_icon->flags.bits.indirected = 1;
    p_icon->flags.bits.filled = 1;
    p_icon->flags.bits.horz_centred = 1;
    p_icon->flags.bits.button_type = wimp_BCLICKDEBOUNCE;
    p_icon->flags.bits.fg_colour = 7;
    p_icon->flags.bits.bg_colour = 0;

    p_icon->data.is.sprite = iconbar_spritename;
    p_icon->data.is.sprite_area = resspr_area();
    p_icon->data.is.sprite_name_length = strlen(iconbar_spritename);

}

static _kernel_oserror *
iconbar_new_sprite(
    const char * spritename)
{
    WimpCreateIconBlock create;

    xstrkpy(iconbar_spritename, elemof32(iconbar_spritename), spritename);

    create.window_handle = -1; /* icon bar, right hand side */
    iconbar_new_sprite_setup((WimpIconBlockWithBitset *) &create.icon);

    return(tbl_wimp_create_icon(0, &create, &iconbar_icon_handle));
}

/******************************************************************************
*
* initialise RISC OS icon bar system
*
******************************************************************************/

static void
iconbar_initialise(
    const char *appname)
{
    static char iconbar_spritename[16] = "!";

    trace_1(TRACE_APP_PD4, "iconbar_initialise(%s)", appname);

    /* make name of icon bar icon sprite */
    xstrkat(iconbar_spritename, elemof32(iconbar_spritename), appname);

    winx_register_new_event_handler(win_ICONBAR,
                                    default_event_handler,
                                    NULL);

    /* new icon (sprite/no text) */
    void_WrapOsErrorReporting(iconbar_new_sprite(iconbar_spritename));
}

/******************************************************************************
*
* do a little drawing in the background
*
******************************************************************************/

static void
continue_draw(void)
{
    P_DOCU p_docu;

    for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
    {
        if(docu_is_thunk(p_docu))
            continue;

        if(p_docu->Xxf_interrupted)
        {
            trace_1(TRACE_NULL, "continuing with interrupted draw for document %d", p_docu->docno);

            select_document(p_docu);

            /* do draw_screen first to get new caret coords */
            draw_screen();

            if(!xf_acquirecaret)
                break;            /* only do one per event */
        }

        if(p_docu->Xxf_acquirecaret)
        {
            trace_1(TRACE_NULL, "continuing with caret acquisition for document %d", p_docu->docno);

            select_document(p_docu);

            xf_acquirecaret = FALSE;
            xf_caretreposition = TRUE;
            draw_caret();

            break;            /* only do one per event */
        }
    }
}

/******************************************************************************
*
*  see if anyone needs null events
*
******************************************************************************/

static void
check_if_null_events_wanted(
    _InVal_     int event_code)
{
    BOOL required = FALSE;

    if(Wimp_ENull != event_code)
        required = TRUE; /* always schedule a null event to work out whether they are needed! */

    if(!required)
    {
        required = (ev_todo_check() > 0);
        trace_1(TRACE_NULL, "nulls wanted for recalc: %s", report_boolstring(required));
    }

    if(!required)
    {
        P_DOCU p_docu;

        for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
        {
            if(docu_is_thunk(p_docu))
                continue;

            if(p_docu->Xxf_interrupted)
            {
                trace_0(TRACE_NULL, "nulls wanted for interrupted drawing: ");
                required = TRUE;
                break;
            }

            if(p_docu->Xxf_acquirecaret)
            {
                trace_0(TRACE_NULL, "nulls wanted for caret acquisition: ");
                required = TRUE;
                break;
            }
        }
    }

    if(!required)
        if(browsing_docno != DOCNO_NONE)
        {
            trace_0(TRACE_NULL, "nulls wanted for browse: ");
            required = TRUE;
        }

    if(!required)
        if(pausing_docno != DOCNO_NONE)
        {
            trace_0(TRACE_NULL, "nulls wanted for pause: ");
            required = TRUE;
        }

    win_claim_idle_events(required ? win_ICONBARLOAD : win_IDLE_OFF);
}

/******************************************************************************
*
* Tidy up before returning control back to the Window Manager
*
******************************************************************************/

static void
actions_when_idle_reached(void)
{
    /* anyone need end of data sending? */
    send_end_markers();
}

static void
actions_before_exiting(wimp_eventstr * e)
{
    /* anyone need background events? */
    check_if_null_events_wanted(e->e);

    if(Wimp_ENull == e->e)
    {   /* do all the things to do once we have settled down */
        actions_when_idle_reached();
    }

    /* forget about any fonts we have handles on (but don't kill all of the cache mechanism) */
    font_close_all(FALSE);

    /* destroy static base pointer */
    select_document(NO_DOCUMENT);

    /* everything processed here */
    been_error = FALSE;
}

/******************************************************************************
*
* tell client this sheet is no longer open
*
******************************************************************************/

extern void
riscos_sendsheetclosed(
    _InRef_opt_ PC_GRAPHICS_LINK_ENTRY glp)
{
    WimpMessageExtra user_message;

    if(glp)
    {
        user_message.hdr.size   = offsetof(WimpMessageExtra, data.pd_dde.type.b)
                                + sizeof(WIMP_MSGPD_DDETYPEB);
        user_message.hdr.my_ref = 0;        /* fresh msg */
        user_message.hdr.action_code = Wimp_MPD_DDE;

        user_message.data.pd_dde.id            = Wimp_MPD_DDE_SheetClosed;
        user_message.data.pd_dde.type.b.handle = glp->ghan;

        void_WrapOsErrorReporting(wimp_send_message_to_task(Wimp_EUserMessage, &user_message, glp->task));
    }
}

/******************************************************************************
*
* send client the contents of a cell
*
******************************************************************************/

extern void
riscos_sendcellcontents(
    _InoutRef_opt_ P_GRAPHICS_LINK_ENTRY glp,
    S32 x_off,
    S32 y_off)
{
    if(glp)
    {
        ROW row;
        char buffer[LIN_BUFSIZ];
        const char *ptr;
        char *to, ch;
        S32 nbytes;
        WIMP_MSGPD_DDETYPEC_TYPE type;
        WimpMessageExtra user_message;
        P_CELL tcell;

        glp->datasent = TRUE;

        nbytes = 0;
        type = Wimp_MPD_DDE_typeC_type_Text;
        row = glp->row + y_off;
        ptr = NULL;

        tcell = travel(glp->col + x_off, row);

        if(tcell)
        {
            P_EV_RESULT p_ev_result;

            if(result_extract(tcell, &p_ev_result) == SL_NUMBER)
            {
                switch(p_ev_result->data_id)
                {
                case DATA_ID_LOGICAL:
                case DATA_ID_WORD16:
                case DATA_ID_WORD32:
                    user_message.data.pd_dde.type.c.content.number = (F64) p_ev_result->arg.integer;
                    goto send_number;

                case DATA_ID_REAL:
                    user_message.data.pd_dde.type.c.content.number = p_ev_result->arg.fp;
                send_number:
                    type = Wimp_MPD_DDE_typeC_type_Number;
                    nbytes = sizeof(F64);
                    break;

                default:
                    break;
                }
            }

            if(0 == nbytes)
            {
                S32 t_justify, t_format, t_opt_DF, t_opt_TH;

                t_justify = tcell->justify;
                tcell->justify = J_LEFT;
                if(tcell->type == SL_NUMBER)
                {
                    t_format  = tcell->format;
                    tcell->format = t_format & ~(F_LDS | F_TRS | F_BRAC);
                }
                else
                    t_format = 0; /* keep compiler dataflow analyser happy */
                t_opt_DF  = d_options_DF;
                d_options_DF = 'E';
                /* ensure no silly commas etc. output! */
                t_opt_TH  = d_options_TH;
                d_options_TH = TH_BLANK;
                curpnm = 0;        /* we have really no idea */

                (void) expand_cell(
                            current_docno(), tcell, row, buffer, LIN_BUFSIZ,
                            DEFAULT_EXPAND_REFS /*expand_refs*/,
                            EXPAND_FLAGS_EXPAND_ATS_ALL /*expand_ats*/ |
                            EXPAND_FLAGS_EXPAND_CTRL /*expand_ctrl*/ |
                            EXPAND_FLAGS_DONT_ALLOW_FONTY_RESULT /*!allow_fonty_result*/ /*expand_flags*/,
                            TRUE /*cff*/);

                tcell->justify    = t_justify;
                if(tcell->type == SL_NUMBER)
                    tcell->format = t_format;
                d_options_DF      = t_opt_DF;
                d_options_TH      = t_opt_TH;

                /* remove highlights & funny spaces etc. from plain non-fonty string */
                ptr = buffer;
                to  = buffer;
                do  {
                    ch = *ptr++;
                    if((ch >= SPACE)  ||  !ch)
                        *to++ = ch;
                }
                while(ch);
                ptr = buffer;
            }
        }
        else
            ptr = NULLSTR;

        if(ptr)
        {
            xstrkpy(user_message.data.pd_dde.type.c.content.text, elemof32(user_message.data.pd_dde.type.c.content.text), ptr);
            nbytes = strlen32p1(user_message.data.pd_dde.type.c.content.text);
        }

        user_message.hdr.size   = offsetof(WimpMessageExtra, data.pd_dde.type.c.content)
                                + nbytes;
        user_message.hdr.my_ref = 0;        /* fresh msg */
        user_message.hdr.action_code = Wimp_MPD_DDE;

        user_message.data.pd_dde.id            = Wimp_MPD_DDE_SendSlotContents;
        user_message.data.pd_dde.type.c.handle = glp->ghan;
        user_message.data.pd_dde.type.c.x_off  = x_off;
        user_message.data.pd_dde.type.c.y_off  = y_off;
        user_message.data.pd_dde.type.c.type   = type;

        trace_2(TRACE_MODULE_UREF, "sendcellcontents x:%d, y:%d", x_off, y_off);

        void_WrapOsErrorReporting(wimp_send_message_to_task(Wimp_EUserMessageRecorded, &user_message, glp->task));
    }
}

extern void
riscos_sendallcells(
    _InoutRef_  P_GRAPHICS_LINK_ENTRY glp)
{
    S32 xoff, yoff;

    xoff = 0;
    do  {
        yoff = 0;
        do  {
            riscos_sendcellcontents(glp, xoff, yoff);
        }
        while(++yoff <= glp->y_size);
    }
    while(++xoff <= glp->x_size);
}

/******************************************************************************
*
*  send client an end of data message
*
******************************************************************************/

static void
riscos_sendendmarker(
    _InoutRef_opt_ P_GRAPHICS_LINK_ENTRY glp)
{
    WimpMessageExtra user_message;

    if(glp)
    {
        glp->datasent = FALSE;

        user_message.hdr.size   = offsetof(WimpMessageExtra, data.pd_dde.type.c)
                                + sizeof(WIMP_MSGPD_DDETYPEC);
        user_message.hdr.my_ref = 0;        /* fresh msg */
        user_message.hdr.action_code = Wimp_MPD_DDE;

        user_message.data.pd_dde.id            = Wimp_MPD_DDE_SendSlotContents;
        user_message.data.pd_dde.type.c.handle = glp->ghan;
        user_message.data.pd_dde.type.c.type   = Wimp_MPD_DDE_typeC_type_End;

        void_WrapOsErrorReporting(wimp_send_message_to_task(Wimp_EUserMessageRecorded, &user_message, glp->task));
    }
}

static void
send_end_markers(void)
{
    P_LIST lptr;

    for(lptr = first_in_list(&graphics_link_list);
        lptr;
        lptr = next_in_list(&graphics_link_list))
    {
        P_GRAPHICS_LINK_ENTRY glp = (P_GRAPHICS_LINK_ENTRY) lptr->value;

        if(glp->datasent)
            riscos_sendendmarker(glp);
    }
}

static os_error *
_kernel_fwrite0(
    int os_file_handle,
    P_U8 p_u8)
{
   U8 ch;

   while((ch = *p_u8++) != CH_NULL)
       if(_kernel_ERROR == _kernel_osbput(ch, os_file_handle))
           return((os_error *) _kernel_last_oserror());

    return(NULL);
}

/******************************************************************************
*
* acknowledge a message to its sender
*
******************************************************************************/

static void
acknowledge_User_Message(
    /*acked*/ WimpMessage * const user_message)
{
    trace_0(TRACE_RISCOS_HOST, "acknowledging message: ");

    user_message->hdr.your_ref = user_message->hdr.my_ref;

    void_WrapOsErrorReporting(wimp_send_message_to_task(Wimp_EUserMessageAcknowledge, user_message, user_message->hdr.sender));
}

/******************************************************************************
*
* derived from strncatind, but returns pointer to CH_NULL byte at end of a
* takes indirect count, which is updated
*
******************************************************************************/

static P_U8
strnpcpyind(
    P_U8 a,
    PC_U8 b,
    size_t * np /*inout*/)
{
    size_t n = *np;
    char   c;

    if(n)
    {
        while(n-- > 0)
        {
            c = *b++;
            if(!c)
                break;
            *a++ = c;
        }

        *a  = CH_NULL;
        *np = n;
    }

    return(a);
}

/******************************************************************************
*
*  event processing for broadcasts
*  and other such events that don't go
*  to a main window handler
*
******************************************************************************/

/******************************************************************************
*
* process message events
*
******************************************************************************/

/* initial drag of data from somewhere to icon */

static BOOL
iconbar_event_Message_DataSave(
    const WimpMessage * const user_message)
{
    scraptransfer_file(&user_message->data.data_save, TRUE /* iconbar */);

    return(TRUE);
}

static void
iconbar_event_Message_DataLoad_PipeDream(
    _In_z_      char * filename,
    _InVal_     FILETYPE_RISC_OS filetype)
{
    DOCNO docno = find_document_using_wholename(filename);

    if(DOCNO_NONE != docno)
    {
        front_document_using_docno(docno);
    }
    else if(FILETYPE_PD_MACRO == filetype)
    {
        STATUS filetype_option = 'T';

        (void) riscos_LoadFile(filename, FALSE, filetype_option /*may be 0,Err*/);
    }
    else
    {
        STATUS filetype_option = find_filetype_option(filename, filetype); /* checks readability and discriminates PipeDream chart files */

        (void) riscos_LoadFile(filename, FALSE, filetype_option /*may be 0,Err*/);
    }

    /* delete NOW if was scrap */
    xferrecv_insertfileok();
}

static void
iconbar_event_Message_DataLoad_others(
    _In_z_      char * filename,
    _InVal_     FILETYPE_RISC_OS filetype)
{
    DOCNO docno = find_document_using_wholename(filename);

    if(DOCNO_NONE != docno)
    {
        front_document_using_docno(docno);
    }
    else if(image_cache_can_import(filetype))
    {
        trace_0(TRACE_APP_PD4, "ignore image file as we can't do anything sensible");
    }
    else
    {   /* loading other file as new file */
        STATUS filetype_option = find_filetype_option(filename, filetype); /* check the readability & do the auto-detect here for consistency */

        (void) riscos_LoadFile(filename, FALSE, filetype_option /*may be 0,Err*/);
    }

    /* delete NOW if was scrap */
    xferrecv_insertfileok();
}

/* object dragged to icon bar - load mostly regardless of type */

static BOOL
iconbar_event_Message_DataLoad(
    const WimpMessage * const user_message)
{
    char * filename;
    const FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkinsert(&filename);

    UNREFERENCED_PARAMETER_InRef_(user_message); /* xferrecv uses last message mechanism */

    reportf("Message_DataLoad(icon): file type &%03X, name %u:%s", filetype, strlen32(filename), filename);

    switch(filetype)
    {
    case FILETYPE_DIRECTORY:
    case FILETYPE_APPLICATION:
        reperr(FILE_ERR_ISADIR, filename);
        break;

    case FILETYPE_PD_CHART:
    case FILETYPE_PD_MACRO:
    case FILETYPE_PIPEDREAM:
        iconbar_event_Message_DataLoad_PipeDream(filename, filetype);
        break;

    default:
        iconbar_event_Message_DataLoad_others(filename, filetype);
        break;
    }

    return(TRUE);
}

/* double-click on object */

static void
iconbar_event_Message_DataOpen_PDMacro(
    char * filename)
{
    if(mystr_set(&d_macro_file[0].textfield, filename))
    {
        exec_file(d_macro_file[0].textfield);
        str_clr( &d_macro_file[0].textfield);
    }
}

static void
iconbar_event_Message_DataOpen_others(
    char * filename,
    FILETYPE_RISC_OS filetype)
{
    STATUS filetype_option = find_filetype_option(filename, filetype); /* check the readability & do the auto-detect here for consistency */

    (void) riscos_LoadFile(filename, FALSE, filetype_option /*may be 0,Err*/);
}

static BOOL
iconbar_event_Message_DataOpen(
    const WimpMessage * const user_message)
{
    char * filename;
    const FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkinsert(&filename);
    DOCNO docno;

    UNREFERENCED_PARAMETER_InRef_(user_message); /* xferrecv uses last message mechanism */

    trace_1(TRACE_APP_PD4, "ukprocessor got asked if it can 'run' a file of type &%03X", filetype);

    if(!pd_can_run(filetype))
        return(TRUE);

    reportf("Message_DataOpen: file type &%03X, name %u:%s", filetype, strlen32(filename), filename);

    /* if it's a macro file we need this here to stop pause in macro file allowing message bounce
     * thereby allowing Filer to try to invoke another copy of PipeDream to run this macro file ...
     * and it helps anyway.
     */
    xferrecv_insertfileok();

    docno = find_document_using_wholename(filename);

    if(DOCNO_NONE != docno)
    {
        front_document_using_docno(docno);
    }
    else if(FILETYPE_PD_MACRO == filetype)
    {
        iconbar_event_Message_DataOpen_PDMacro(filename);
    }
    else
    {
        iconbar_event_Message_DataOpen_others(filename, filetype);
    }

    return(TRUE);
}

/* We assume that the sender is the Task Manager, so that
 * Ctrl-Shift-F12 is the Desktop Shutdown key sequence.
 */

static void
riscos_send_CSF12(
    _InVal_     wimp_t sender)
{
    WimpPollBlock event_data;
    void_WrapOsErrorReporting(tbl_wimp_get_caret_position(&event_data.key_pressed.caret));
    event_data.key_pressed.key_code = akbd_Sh + akbd_Ctl + akbd_Fn12;
    void_WrapOsErrorReporting(wimp_send_message_to_task(Wimp_EKeyPressed, &event_data, sender));
}

static BOOL
iconbar_event_Message_PreQuit(
    /*acked*/ WimpMessageExtra * const user_message)
{
    const wimp_t sender = user_message->hdr.sender; /* caller's task id */
    const int flags = user_message->data.prequit.flags & Wimp_MPREQUIT_flags_killthistask;
    int count;

    mergebuf_all(); /* ensure modified buffers to docs */

    count = documents_modified();

    /* if any are modified, it gets harder... */
    if(0 == count)
        return(TRUE);

    /* First, acknowledge the message to stop it going any further */
    acknowledge_User_Message((WimpMessage *) user_message);

    /* And then ask the user what to do */
    if(riscos_quit_okayed(count))
    {
        if(flags)
        {   /* We are being killed individually by the Task Manager */
            /* Do NOT send back a C/S/F12 */
            exit(EXIT_SUCCESS);
        }

        /* Start up the Shutdown sequence again if OK and all tasks are to die */
        riscos_send_CSF12(sender);
    }

    return(TRUE);
}

static BOOL
iconbar_event_Message_PaletteChange(void)
{
    cache_palette_variables();

    draw_redraw_all_pictures();

    return(TRUE);
}

/* SKS 26oct96 added */

static BOOL
iconbar_event_Message_SaveDesktop(
    /*acked*/ WimpMessage * const user_message)
{
    const int os_file_handle = user_message->data.save_desktop.file_handle;
    os_error * e = NULL;
    char var_name[BUF_MAX_PATHSTRING];
    TCHARZ main_dir[BUF_MAX_PATHSTRING];
    TCHARZ user_path[BUF_MAX_PATHSTRING];
    P_U8 p_main_app = main_dir;

    main_dir[0] = CH_NULL;
    (void) _kernel_getenv(make_var_name(var_name, elemof32(var_name), "$Dir"), main_dir, elemof32(main_dir));

    user_path[0] = CH_NULL;
    (void) _kernel_getenv(make_var_name(var_name, elemof32(var_name), "$UserPath"), user_path, elemof32(user_path));
    /* Ignore PRM guideline about caching at startup; final seen one is most relevant */

    /* write out commands to desktop save file that will restart app */
    if(user_path[0] != CH_NULL)
    {
        P_U8 p_u8;

        /* need to boot main app first */
        if((e = _kernel_fwrite0(os_file_handle, "Filer_Boot ")) == NULL)
        if((e = _kernel_fwrite0(os_file_handle, p_main_app)) == NULL)
            e = _kernel_fwrite0(os_file_handle, "\x0A");

        p_u8 = user_path + strlen(user_path);
        assert(p_u8[-1] == ',');
        assert(p_u8[-2] == '.');
        p_u8[-2] = CH_NULL;

        p_main_app = user_path;
    }

    if(NULL == e)
    {
        /* now run app */
        if((e = _kernel_fwrite0(os_file_handle, "Run ")) == NULL)
        if((e = _kernel_fwrite0(os_file_handle, p_main_app)) == NULL)
            e = _kernel_fwrite0(os_file_handle, "\x0A");
    }

    if(NULL != e)
    {
        void_WrapOsErrorReporting(e);

        /* ack the message to stop desktop save and remove file */
        acknowledge_User_Message(user_message);
    }

    return(TRUE);
}

static BOOL
iconbar_event_Message_HelpRequest(
    /*acked*/ WimpMessage * const user_message)
{
    trace_0(TRACE_APP_PD4, "ukprocessor got Help request for icon bar icon");

    user_message->data.help_request.icon_handle = 0;

    riscos_send_Message_HelpReply(user_message, help_iconbar);

    return(TRUE);
}

static BOOL
iconbar_event_Message_ModeChange(void)
{
    cache_mode_variables();

    return(TRUE);
}

static BOOL
iconbar_event_Message_TaskInitialise(
    const WimpMessageExtra * const user_message)
{
    if(g_kill_duplicates)
    {
        static int seen_my_birth = FALSE;
        const char *taskname = user_message->data.task_initialise.taskname;
        trace_1(TRACE_APP_PD4, "Message_TaskInitialise for %s", taskname);
        if(0 == strcmp(wimptx_get_taskname(), taskname))
        {
            if(seen_my_birth)
            {
                WimpUserMessageEvent user_message_quit;
                trace_1(TRACE_APP_PD4, "Another %s is trying to start! I'll kill it", taskname);
                /* no need to clear all fields */
                user_message_quit.hdr.size = sizeof(user_message_quit);
                user_message_quit.hdr.your_ref = 0; /* fresh msg */
                user_message_quit.hdr.action_code = Wimp_MQuit;
                void_WrapOsErrorReporting(wimp_send_message_to_task(Wimp_EUserMessage, &user_message_quit, user_message->hdr.sender));
            }
            else
            {
                trace_0(TRACE_APP_PD4, "witnessing our own birth");
                seen_my_birth = TRUE;
            }
        }
    }

    return(TRUE);
}

static BOOL
iconbar_event_Message_PrintTypeOdd(void)
{
    /* printer broadcast */
    char * filename;
    FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkprint(&filename);

    trace_1(TRACE_APP_PD4, "ukprocessor got asked if it can print a file of type &%03X", filetype);

    if(pd_can_print(filetype))
    {
        print_file(filename);

        xferrecv_printfileok(-1);    /* print all done */
    }

    return(TRUE);
}

static BOOL
iconbar_event_Message_SetPrinter(void)
{
    trace_0(TRACE_APP_PD4, "ukprocessor got informed of printer driver change");

    riscprint_set_printer_data();

    return(TRUE);
}

static BOOL
iconbar_event_Message_PD_DDE(
    /*acked*/ WimpMessageExtra * const user_message)
{
    BOOL processed = TRUE;
    const int sender = (int) user_message->hdr.sender; /* caller's task id */
    const S32 pd_dde_id = user_message->data.pd_dde.id;

    trace_1(TRACE_APP_PD4, "ukprocessor got PD DDE message %d", pd_dde_id);

    switch(pd_dde_id)
    {
    case Wimp_MPD_DDE_IdentifyMarkedBlock:
        {
        trace_0(TRACE_APP_PD4, "IdentifyMarkedBlock");

        if((blkstart.col != NO_COL)  &&  (blkend.col != NO_COL))
        {
            WimpMessageExtra user_message_reply;
            char *ptr = user_message_reply.data.pd_dde.type.a.text;
            size_t nbytes = sizeof(WIMP_MSGPD_DDETYPEA_TEXT)-1;
            ghandle ghan;
            S32 x_size = (S32) (blkend.col - blkstart.col);
            S32 y_size = (S32) (blkend.row - blkstart.row);
            const char *leaf;
            const char *tag;
            S32 taglen;
            P_CELL tcell;

            select_document_using_docno(blk_docno);

            (void) mergebuf_nocheck();
            filbuf();

            leaf = file_leafname(currentfilename());
            ptr = strnpcpyind(ptr, leaf, &nbytes);

            tcell = travel(blkstart.col, blkstart.row);

            if(tcell  &&  (tcell->type == SL_TEXT))
                tag = tcell->content.text;
            else
                tag = ambiguous_tag_STR;

            taglen = strlen(tag);

            if((size_t) taglen < nbytes)
            {
                /* need to copy tag before adding to list to avoid moving memory problems */
                strcpy(ptr, tag);
                tag = ptr;
                ptr += taglen + 1;

                /* create entry on list - even if already there */
                ghan = graph_add_entry(user_message->hdr.my_ref,        /* unique number */
                                       blk_docno, blkstart.col, blkstart.row,
                                       x_size, y_size, leaf, tag,
                                       sender);

                if(ghan > 0)
                {
                    trace_5(TRACE_APP_PD4, "IMB: ghan %d x_size %d y_size %d leafname %s tag %s]",
                            ghan, x_size, y_size, leaf, tag);

                    user_message_reply.data.pd_dde.id            = Wimp_MPD_DDE_ReturnHandleAndBlock;
                    user_message_reply.data.pd_dde.type.a.handle = ghan;
                    user_message_reply.data.pd_dde.type.a.x_size = x_size;
                    user_message_reply.data.pd_dde.type.a.y_size = y_size;

                    /* send message reply as ack to his one */
                    user_message_reply.hdr.size     = ptr - (char *) &user_message_reply;
                    user_message_reply.hdr.your_ref = user_message->hdr.my_ref;
                    user_message_reply.hdr.action_code = Wimp_MPD_DDE;
                    void_WrapOsErrorReporting(wimp_send_message_to_task(Wimp_EUserMessageRecorded, &user_message_reply, sender));
                }
                else if(ghan  &&  (ghan != status_nomem()))
                    reperr_null(ghan);
                else
                    trace_0(TRACE_APP_PD4, "IMB: failed to create ghandle - caller will get bounced msg");
            }
            else
                trace_0(TRACE_APP_PD4, "IMB: leafname/tag too long - caller will get bounced msg");
        }
        else
            trace_0(TRACE_APP_PD4, "IMB: no block marked/only one mark set - caller will get bounced msg");
        }
        break;

    case Wimp_MPD_DDE_EstablishHandle:
        {
        ghandle ghan;
        S32 x_size = user_message->data.pd_dde.type.a.x_size;
        S32 y_size = user_message->data.pd_dde.type.a.y_size;
        const char *tstr = user_message->data.pd_dde.type.a.text;
        const char *tag  = tstr + strlen32p1(tstr);
        DOCNO docno;
        COL col;
        ROW row;
        P_CELL tcell;

        trace_4(TRACE_APP_PD4, "EstablishHandle: x_size %d, y_size %d, name %s, tag %s", x_size, y_size, tstr, tag);

        if(file_is_rooted(tstr))
        {
            docno = find_document_using_wholename(tstr);
        }
        else
        {
            docno = find_document_using_leafname(tstr);

            if(DOCNO_SEVERAL == docno)
                break;
        }

        if(DOCNO_NONE == docno)
            break;

        select_document_using_docno(docno);

        (void) mergebuf_nocheck();
        filbuf();

        /* find tag in document */
        init_doc_as_block();

        while((tcell = next_cell_in_block(DOWN_COLUMNS)) != NULL)
        {
            if(tcell->type != SL_TEXT)
                continue;

            if(0 != C_stricmp(tcell->content.text, tag))
                continue;

            col = in_block.col;
            row = in_block.row;

            /* add entry to list */
            ghan = graph_add_entry(user_message->hdr.my_ref,        /* unique number */
                                   docno, col, row,
                                   x_size, y_size, tstr, tag,
                                   sender);

            if(ghan > 0)
            {
                trace_5(TRACE_APP_PD4, "EST: ghan %d x_size %d y_size %d name %s tag %s]",
                        ghan, x_size, y_size, tstr, tag);

                user_message->data.pd_dde.id            = Wimp_MPD_DDE_ReturnHandleAndBlock;
                user_message->data.pd_dde.type.a.handle = ghan;

                /* send same message as ack to his one */
                user_message->hdr.your_ref = user_message->hdr.my_ref;
                user_message->hdr.action_code = Wimp_MPD_DDE;
                void_WrapOsErrorReporting(wimp_send_message_to_task(Wimp_EUserMessageRecorded, user_message, sender));
            }
            else
                trace_0(TRACE_APP_PD4, "EST: failed to create ghandle - caller will get bounced msg");

            break;
        }
        }
        break;

    case Wimp_MPD_DDE_RequestUpdates:
    case Wimp_MPD_DDE_StopRequestUpdates:
        {
        ghandle han = user_message->data.pd_dde.type.b.handle;
        P_GRAPHICS_LINK_ENTRY glp;

        trace_2(TRACE_APP_PD4, "%sRequestUpdates: handle %d", (pd_dde_id == Wimp_MPD_DDE_StopRequestUpdates) ? "Stop" : "", han);

        glp = graph_search_list(han);

        if(glp)
        {
            /* stop caller getting bounced msg */
            acknowledge_User_Message((WimpMessage *) user_message);

            /* flag that updates are/are not required on this handle */
            glp->update = (pd_dde_id == Wimp_MPD_DDE_RequestUpdates);
        }
        break;
        }

    case Wimp_MPD_DDE_RequestContents:
        {
        ghandle han = user_message->data.pd_dde.type.b.handle;
        P_GRAPHICS_LINK_ENTRY glp;

        trace_1(TRACE_APP_PD4, "RequestContents: handle %d", han);

        glp = graph_search_list(han);

        if(glp)
        {
            /* stop caller getting bounced msg */
            acknowledge_User_Message((WimpMessage *) user_message);

            select_document_using_docno(glp->docno);

            (void) mergebuf_nocheck();
            filbuf();

            /* fire off all the cells */
            riscos_sendallcells(glp);

            riscos_sendendmarker(glp);
        }
        break;
        }

    case Wimp_MPD_DDE_GraphClosed:
        {
        ghandle han = user_message->data.pd_dde.type.b.handle;
        PC_GRAPHICS_LINK_ENTRY glp;

        trace_1(TRACE_APP_PD4, "GraphClosed: handle %d", han);

        glp = graph_search_list(han);

        if(glp)
        {
            /* stop caller getting bounced msg */
            acknowledge_User_Message((WimpMessage *) user_message);

            /* delete entry from list */
            graph_remove_entry(han);
        }
        break;
        }

    case Wimp_MPD_DDE_DrawFileChanged:
        {
        const char *drawfilename = user_message->data.pd_dde.type.d.leafname;

        trace_1(TRACE_APP_PD4, "DrawFileChanged: name %s", drawfilename);

        /* don't ack this message: other people may want to see it too */

        /* look for any instances of this DrawFile; update windows with refs */
        image_cache_recache(drawfilename);
        }
        break;

    default:
        trace_1(TRACE_APP_PD4, "ignoring PD DDE type %d message", pd_dde_id);
        processed = FALSE;
        break;
    }

    return(processed);
}

static BOOL
iconbar_event_User_Message(
    /*acked*/ WimpMessage * const user_message)
{
    switch(user_message->hdr.action_code)
    {
    case Wimp_MDataSave:
        return(iconbar_event_Message_DataSave(user_message));

    case Wimp_MDataLoad:
        return(iconbar_event_Message_DataLoad(user_message));

    case Wimp_MDataOpen:
        return(iconbar_event_Message_DataOpen(user_message));

    case Wimp_MPreQuit:
        return(iconbar_event_Message_PreQuit((WimpMessageExtra *) user_message));

    case Wimp_MPaletteChange:
        return(iconbar_event_Message_PaletteChange());

    case Wimp_MSaveDesktop:
        return(iconbar_event_Message_SaveDesktop(user_message));

    case Wimp_MHelpRequest:
        return(iconbar_event_Message_HelpRequest(user_message));

    case Wimp_MModeChange:
        return(iconbar_event_Message_ModeChange());

    case Wimp_MTaskInitialise:
        return(iconbar_event_Message_TaskInitialise((WimpMessageExtra *) user_message));

    case Wimp_MPrintTypeOdd:
        return(iconbar_event_Message_PrintTypeOdd());

    case Wimp_MSetPrinter:
        return(iconbar_event_Message_SetPrinter());

    case Wimp_MPD_DDE:
        return(iconbar_event_Message_PD_DDE((WimpMessageExtra *) user_message));

    default:
        trace_1(TRACE_APP_PD4, "unprocessed %s message to iconbar handler",
                report_wimp_message(user_message, FALSE));
        return(FALSE);
    }
}

static BOOL
iconbar_PD_DDE_bounced(
    WimpMessageExtra * const user_message)
{
    const S32 pd_dde_id = user_message->data.pd_dde.id;

    trace_1(TRACE_APP_PD4, "ukprocessor got bounced PD DDE message %d", pd_dde_id);

    switch(pd_dde_id)
    {
    case Wimp_MPD_DDE_SendSlotContents:
        {
        ghandle han = user_message->data.pd_dde.type.c.handle;
        P_GRAPHICS_LINK_ENTRY glp;
        trace_1(TRACE_APP_PD4, "SendSlotContents on handle %d bounced - receiver dead", han);
        glp = graph_search_list(han);
        if(glp)
            /* delete entry from list */
            graph_remove_entry(han);
        return(TRUE);
        }

    case Wimp_MPD_DDE_ReturnHandleAndBlock:
        {
        ghandle han = user_message->data.pd_dde.type.a.handle;
        P_GRAPHICS_LINK_ENTRY glp;
        trace_1(TRACE_APP_PD4, "ReturnHandleAndBlock on handle %d bounced - receiver dead", han);
        glp = graph_search_list(han);
        if(glp)
            /* delete entry from list */
            graph_remove_entry(han);
        return(TRUE);
        }

    default:
        trace_1(TRACE_APP_PD4, "ignoring bounced PD DDE type %d message", pd_dde_id);
        return(FALSE);
    }
}

static BOOL
iconbar_event_User_Message_Acknowledge(
    WimpMessage * const user_message)
{
    switch(user_message->hdr.action_code)
    {
    case Wimp_MPD_DDE:
        return(iconbar_PD_DDE_bounced((WimpMessageExtra *) user_message));

    default:
        trace_1(TRACE_APP_PD4, "unprocessed %s bounced message to iconbar handler",
                report_wimp_message(user_message, FALSE));
        return(FALSE);
    }
}

/******************************************************************************
*
* process unhandled events
*
******************************************************************************/

static BOOL
default_event_Null(void)
{
    trace_0(TRACE_NULL, "got a null event");

    if(d_progvars[OR_AM].option == 'A')
        ev_recalc();

    continue_draw();

    if(pausing_docno != DOCNO_NONE)
    {
        select_document_using_docno(pausing_docno);
        pausing_null();
    }

    if(browsing_docno != DOCNO_NONE)
    {
        select_document_using_docno(browsing_docno);
        browse_null();
    }

    return(TRUE);
}

static BOOL
default_event_User_Drag_Box(
    _In_        const WimpUserDragBoxEvent * const user_drag_box)
{
    trace_0(TRACE_APP_PD4, "User_Drag_Box: stopping drag as button released");

    if(DOCNO_NONE == drag_docno)
    {   /* due to wally RISC_OSLib window event processing, we get end-of-drag events for dboxtcol too... */
        assert(drag_type == NO_DRAG_ACTIVE);
        return(TRUE);
    }

    assert(drag_type != NO_DRAG_ACTIVE);

    /* send this to the right guy */
    select_document_using_docno(drag_docno);

    application_drag(user_drag_box->bbox.xmin, user_drag_box->bbox.ymin, TRUE);

    ended_drag(); /* will release nulls */

    return(TRUE);
}

static BOOL
default_event_handler(
    wimp_eventstr *e,
    void *handle)
{
    const int event_code = (int) e->e;
    WimpPollBlock * const event_data = (WimpPollBlock *) &e->data;

    UNREFERENCED_PARAMETER(handle);

    select_document(NO_DOCUMENT);

    switch(event_code)
    {
    case Wimp_ENull:
        return(default_event_Null());

    case Wimp_EMouseClick:
        /* one presumes only the icon bar stuff gets here ... */
        return(iconbar_event_Mouse_Click(&event_data->mouse_click));

    case Wimp_EUserDrag:
        return(default_event_User_Drag_Box(&event_data->user_drag_box));

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        return(iconbar_event_User_Message(&event_data->user_message));

    case Wimp_EUserMessageAcknowledge:
        return(iconbar_event_User_Message_Acknowledge(&event_data->user_message));

    default:
        trace_1(TRACE_APP_PD4, "unprocessed wimp event %s",
                report_wimp_event(event_code, event_data));
        return(FALSE);
    }
}

/******************************************************************************
*
*  process open window request for rear window
*
******************************************************************************/

static BOOL
rear_event_Open_Window_Request(
    /*poked*/ WimpOpenWindowRequestEvent * const open_window_request)
{
    trace_0(TRACE_APP_PD4, "rear_event_Open_Window_Request()");

    application_Open_Window_Request(open_window_request);

    return(TRUE);
}

/******************************************************************************
*
* process close window request for rear window
*
******************************************************************************/

/* if       select-close then close window */
/* if       adjust-close then open parent directory and close window */
/* if shift-adjust-close then open parent directory */

static BOOL
rear_event_Close_Window_Request(
    _In_        const WimpCloseWindowRequestEvent * const close_window_request)
{
    BOOL adjust_clicked = riscos_adjust_clicked();
    BOOL shift_pressed  = host_shift_pressed();
    BOOL just_opening   = (shift_pressed  &&  adjust_clicked);
    BOOL want_to_close  = !just_opening;

    UNREFERENCED_PARAMETER_InRef_(close_window_request);

    trace_0(TRACE_APP_PD4, "rear_event_Close_Window_Request()");

    if(!just_opening)
    {
        /* deal with modified files etc. before opening any Filer windows */
        want_to_close = riscdialog_warning();

        (void) mergebuf_nocheck();
        filbuf();

        if(want_to_close)
            want_to_close = save_or_discard_existing();

        if(want_to_close)
            want_to_close = dependent_files_warning();

        if(want_to_close)
            want_to_close = dependent_charts_warning();

        if(want_to_close)
            want_to_close = dependent_links_warning();
    }

    if(adjust_clicked)
        if(file_is_rooted(currentfilename()))
            filer_opendir(currentfilename());

    if(want_to_close)
        close_window();

    return(TRUE);
}

/******************************************************************************
*
* process scroll window request for rear window
*
******************************************************************************/

static BOOL
rear_event_Scroll_Request(
    _In_        const WimpScrollRequestEvent * const scroll_request)
{
    trace_0(TRACE_APP_PD4, "rear_event_Scroll_Request()");

    application_Scroll_Request(scroll_request);

    return(TRUE);
}

/******************************************************************************
*
* process message events for rear window
*
******************************************************************************/

static BOOL
main_event_User_Message(
    /*acked*/ WimpMessage * const user_message);

static BOOL
rear_event_User_Message(
    /*acked*/ WimpMessage * const user_message)
{
    /* SKS 26oct96 delegate handling */
    return(main_event_User_Message(user_message));
}

/******************************************************************************
*
* process main window events
*
******************************************************************************/

extern void
riscos_event_handler_report(
    _InVal_     int event_code,
    _In_        const WimpPollBlock * const event_data,
    void * handle,
    _In_z_      const char * name)
{
    if(reporting_is_enabled())
        reportf(TEXT("%s[handle ") PTR_XTFMT TEXT(" document %d] %s"),
                name, report_ptr_cast(handle), current_docno(), report_wimp_event(event_code, event_data));
}

#define rear_event_handler_report(event_code, event_data, handle) \
    riscos_event_handler_report(event_code, event_data, handle, "rear")

static BOOL
rear_event_handler(
    wimp_eventstr *e,
    void *handle)
{
    const int event_code = (int) e->e;
    WimpPollBlock * const event_data = (WimpPollBlock *) &e->data;

    if(!select_document_using_callback_handle(handle))
    {
        messagef(TEXT("Bad handle ") PTR_XTFMT TEXT(" passed to rear event handler"), report_ptr_cast(handle));
        return(FALSE);
    }

    trace_4(TRACE_APP_PD4, TEXT("rear_event_handler: event %s, handle ") PTR_XTFMT TEXT(" window %d, document %d"),
             report_wimp_event(event_code, event_data), report_ptr_cast(handle), (int) rear_window_handle, current_docno());

    switch(event_code)
    {
    case Wimp_ERedrawWindow:
        return(TRUE);

    case Wimp_EOpenWindow:
        /*rear_event_handler_report(event_code, event_data, handle);*/
        return(rear_event_Open_Window_Request(&event_data->open_window_request));

    case Wimp_ECloseWindow:
        /*rear_event_handler_report(event_code, event_data, handle);*/
        return(rear_event_Close_Window_Request(&event_data->close_window_request));

    case Wimp_EPointerLeavingWindow:
    case Wimp_EPointerEnteringWindow:
        /*rear_event_handler_report(event_code, event_data, handle);*/
        return(TRUE); /* not default: */

    case Wimp_EScrollRequest:
        rear_event_handler_report(event_code, event_data, handle);
        return(rear_event_Scroll_Request(&event_data->scroll_request));

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        /*rear_event_handler_report(event_code, event_data, handle);*/
        return(rear_event_User_Message(&event_data->user_message));

    default:
        /*rear_event_handler_report(event_code, event_data, handle);*/
        trace_1(TRACE_APP_PD4, "unprocessed wimp event to rear window: %s",
                report_wimp_event(event_code, event_data));
        return(FALSE);
    }
}

/******************************************************************************
*
* process redraw window request for main window
*
******************************************************************************/

static BOOL
main_event_Redraw_Window_Request(
    const WimpRedrawWindowRequestEvent * const redraw_window_request)
{
    WimpRedrawWindowBlock redraw_window_block;
    BOOL more;

    trace_0(TRACE_APP_PD4, "main_event_Redraw_Window_Request()");

    redraw_window_block.window_handle = redraw_window_request->window_handle;

    /* wimp errors in redraw are fatal */
    if(NULL != WrapOsErrorReporting(tbl_wimp_redraw_window(&redraw_window_block, &more)))
        more = FALSE;

    while(more)
    {
        killcolourcache();

        graphics_window = * ((PC_GDI_BOX) &redraw_window_block.redraw_area);

        #if defined(ALWAYS_PAINT)
        paint_is_update = TRUE;
        #else
        paint_is_update = FALSE;
        #endif

        /* redraw area, not update */
        application_redraw_core((const RISCOS_RedrawWindowBlock *) &redraw_window_block);

        if(NULL != WrapOsErrorReporting(tbl_wimp_get_rectangle(&redraw_window_block, &more)))
            more = FALSE;
    }

    return(TRUE);
}

/******************************************************************************
*
* process mouse click event for main window
*
******************************************************************************/

static BOOL
main_event_Mouse_Click(
    const WimpMouseClickEvent * const mouse_click)
{
    application_main_Mouse_Click(mouse_click->mouse_x,
                                 mouse_click->mouse_y,
                                 mouse_click->buttons);

    return(TRUE);
}

/******************************************************************************
*
* process key pressed event for main window
*
******************************************************************************/

#define CTRL_H    ('H' & 0x1F)    /* Backspace key */
#define CTRL_M    ('M' & 0x1F)    /* Return/Enter keys */
#define CTRL_Z    ('Z' & 0x1F)    /* Last Ctrl-x key */

_Check_return_
static KMAP_CODE
cs_mutate_key(
    KMAP_CODE kmap_code)
{
    const KMAP_CODE shift_added = host_shift_pressed() ? KMAP_CODE_ADDED_SHIFT : 0;
    const KMAP_CODE ctrl_added  = host_ctrl_pressed()  ? KMAP_CODE_ADDED_CTRL  : 0;

#if CHECKING
    switch(kmap_code)
    {
    case KMAP_FUNC_DELETE:
    case KMAP_FUNC_HOME:
    case KMAP_FUNC_BACKSPACE:
        break;

    default: default_unhandled();
        break;
    }
#endif

    kmap_code |= (shift_added | ctrl_added);

    return(kmap_code);
}

_Check_return_
static KMAP_CODE
convert_ctrl_key(
    _InVal_     S32 code)
{
    KMAP_CODE kmap_code = code;

    if(code <= CTRL_Z)
    {
        /* Watch out for useful CtrlChars not produced by Ctrl-letter */

        /* SKS after PD 4.11 10jan92 - see notes in convert_standard_key() about ^H, ^M */
        if( ((code == CTRL_H)  ||  (code == CTRL_M))  &&
            cmd_seq_is_empty()  &&
            !host_ctrl_pressed() )
        {
            if(code == CTRL_H)
                kmap_code = RISCOS_EKEY_BACKSPACE;
            else /* code == CTRL_M */
            {
                if(host_shift_pressed())
                    kmap_code |= KMAP_CODE_ADDED_SHIFT;
            }
        }
        else if(classic_menus())
        {
            kmap_code = (code - 1 + 'A') | KMAP_CODE_ADDED_ALT; /* Uppercase Alt-letter */
        }
        else
        {
            kmap_code = (code - 1 + 'A') | KMAP_CODE_ADDED_CTRL; /* Uppercase Ctrl-letter */

            if(host_shift_pressed())
                kmap_code |= KMAP_CODE_ADDED_SHIFT;
        }
    }
    else
    {
        switch(code)
        {
        default:
            break;

        case 0x1C: /* Ctrl-Backslash */
            kmap_code = (code - 1 + 'A') | KMAP_CODE_ADDED_ALT; /* Alt-Backslash */
            break;
        }
    }

    /* transform simply produced Delete, Home and Backspace keys into function-like keys */
    switch(kmap_code)
    {
    default:
        break;

    case RISCOS_EKEY_DELETE:
        kmap_code = cs_mutate_key(KMAP_FUNC_DELETE);
        break;

    case RISCOS_EKEY_HOME:
        kmap_code = cs_mutate_key(KMAP_FUNC_HOME);
        break;

    case RISCOS_EKEY_BACKSPACE:
        kmap_code = cs_mutate_key(KMAP_FUNC_BACKSPACE);
        break;
    }

    return(kmap_code);
}

_Check_return_
static KMAP_CODE
convert_standard_key(
    _InVal_     S32 code /*from Wimp_EKeyPressed*/)
{
    KMAP_CODE kmap_code;

    if((code < 0x20) || (code == RISCOS_EKEY_DELETE))
        return(convert_ctrl_key(code)); /* RISC OS 'Hot Key' here we come */

    kmap_code = code;

    /* SKS after PD 4.11 10jan92 - if already in Cmd-sequence then these go in as Alt-letters
     * without considering Ctrl-state (allows ^BM, ^PHB etc. from !Chars but not ^M, ^H still)
     */
    if(!cmd_seq_is_empty()  &&  isalpha(code))
    {   /* already in Cmd-sequence? */
        kmap_code = toupper(code) | KMAP_CODE_ADDED_ALT; /* Uppercase Alt-letter */
    }

    return(kmap_code);
}

/* convert RISC OS Fn keys to our internal representations */

_Check_return_
static KMAP_CODE
convert_function_key(
    _InVal_     S32 code /*from Wimp_EKeyPressed*/)
{
    KMAP_CODE kmap_code = code ^ 0x180; /* map F0 (0x180) to 0x00, F10 (0x1CA) to 0x4A etc. */

    KMAP_CODE shift_added = 0;
    KMAP_CODE ctrl_added  = 0;

    /* remap RISC OS shift and control bits to our definitions */
    if(0 != (kmap_code & 0x10))
    {
        kmap_code ^= 0x10;
        shift_added = KMAP_CODE_ADDED_SHIFT;
    }

    if(0 != (kmap_code & 0x20))
    {
        kmap_code ^= 0x20;
        ctrl_added = KMAP_CODE_ADDED_CTRL;
    }

    /* map F10-F15 range onto end of F0-F9 range ... */
    if((kmap_code >= 0x4A) && (kmap_code <= 0x4F))
    {
        kmap_code ^= (0x00 ^ 0x40);
    }
    /* ... and map TAB-up arrow out of that range */
    else if((kmap_code >= 0x0A) && (kmap_code <= 0x0F))
    {
        kmap_code ^= (0x10 ^ 0x00);
    }

    kmap_code |= (KMAP_CODE_ADDED_FN | shift_added | ctrl_added);

    return(kmap_code);
}

/* Translate key from RISC OS Window manager into what application expects */

_Check_return_
static KMAP_CODE
ri_kmap_convert(
    _InVal_     S32 code /*from Wimp_EKeyPressed*/)
{
    KMAP_CODE kmap_code;

    trace_1(0, TEXT("ri_kmap_convert: key in ") U32_XTFMT, (U32) code);

    switch(code & ~0x000000FF)
    {
    /* 'normal' chars in 0..255 */
    case 0x0000:
        kmap_code = convert_standard_key(code);
        break;

    /* convert RISC OS Fn keys to our internal representations */
    case 0x0100:
        kmap_code = convert_function_key(code);
        break;

    default: default_unhandled();
        kmap_code = 0;
        break;
    }

    trace_2(0, TEXT("ri_kmap_convert(") U32_XTFMT TEXT("): kmap_code out ") U32_XTFMT, (U32) code, (U32) kmap_code);

    return(kmap_code);
}

static BOOL
main_event_Key_Pressed(
    const WimpKeyPressedEvent * const key_pressed)
{
    trace_1(TRACE_APP_PD4, "main_event_Key_Pressed: key &%3.3X", key_pressed->key_code);

    if(application_process_key(ri_kmap_convert(key_pressed->key_code)))
        return(TRUE);

    /* if unprocessed, send it back from whence it came */
    trace_1(TRACE_APP_PD4, "main_key_pressed: unprocessed app_process_key(&%8X)", key_pressed->key_code);
    void_WrapOsErrorReporting(wimp_processkey(key_pressed->key_code));
    return(TRUE);
}

/******************************************************************************
*
* process scroll window request for main window
*
* scroll furniture interactions come through rear window
*
******************************************************************************/

static BOOL
main_event_Scroll_Request(
    _In_        const WimpScrollRequestEvent * const scroll_request)
{
    trace_0(TRACE_APP_PD4, "main_event_Scroll_Request()");

    application_Scroll_Request(scroll_request);

    return(TRUE);
}

/******************************************************************************
*
* process caret lose event for main window
*
******************************************************************************/

static BOOL
main_event_Lose_Caret(
    wimp_eventstr *e)
{
    trace_2(TRACE_APP_PD4, "main_event_Lose_Caret: old window %d icon %d",
            e->data.c.w, e->data.c.i);
    trace_4(TRACE_APP_PD4, " x %d y %d height %X index %d",
            e->data.c.x, e->data.c.y,
            e->data.c.height, e->data.c.index);

    assert(caret_window_handle == e->data.c.w);

    UNREFERENCED_PARAMETER(e);

    cmd_seq_cancel(); /* cancel Cmd-sequence */

    /* don't cancel key expansion or else user won't be able to
     * set 'Next window', 'other action' etc. on keys
    */
    (void) mergebuf_nocheck();
    filbuf();

    caret_stolen_from_window_handle = caret_window_handle;
    caret_window_handle = HOST_WND_NONE;

#if FALSE
    colh_draw_cell_count(NULL);

    /*RCM says: I suppose we could find out which doc (if any) is getting the focus */
    /*          and only blank the cell count if its not this doc - this would mean */
    /*          the count wouldn't blank out momentarily if the focus went to an    */
    /*          edit expression box owned by this document                          */
    /*          NB The expression editor would need similar ELOSECARET code to this */
#endif

    return(TRUE);
}

/******************************************************************************
*
* process caret gain event for main window
*
******************************************************************************/

static BOOL
main_event_Gain_Caret(
    wimp_eventstr *e)
{
    trace_2(TRACE_APP_PD4, "main_event_Gain_Caret: new window %d icon %d",
            e->data.c.w, e->data.c.i);
    trace_4(TRACE_APP_PD4, " x %d y %d height %8.8X index %d",
            e->data.c.x, e->data.c.y,
            e->data.c.height, e->data.c.index);

    caret_window_handle = e->data.c.w;
    caret_stolen_from_window_handle = HOST_WND_NONE;

    /* This document is gaining the caret, and will show the cell count (recalculation status) from now on */
    if(slotcount_docno != current_docno())
    {
        colh_draw_cell_count_in_document(NULL); /* kill the current indicator (if any) */

        slotcount_docno = current_docno();
    }

    return(TRUE);
}

/******************************************************************************
*
* process message events for main window
*
******************************************************************************/

/* possible object dragged into main window - ask for DataLoad */

static BOOL
main_event_Message_DataSave(
    const WimpMessage * const user_message)
{
    scraptransfer_file(&user_message->data.data_save, FALSE /* not iconbar */);

    return(TRUE);
}

static BOOL
draw_insert_filename(
    PC_U8 filename)
{
    char cwd_buffer[BUF_MAX_PATHSTRING];

    trace_0(TRACE_APP_PD4, "inserting minimalist reference to graphic/chart file as text-at G field");

    /* try for minimal reference */
    if(NULL != file_get_cwd(cwd_buffer, elemof32(cwd_buffer), currentfilename()))
    {
        size_t cwd_len = strlen(cwd_buffer);

        if(0 == C_strnicmp(filename, cwd_buffer, cwd_len))
            filename += cwd_len;
    }

    filbuf();

    if(CH_NULL != get_text_at_char())
    {
        if( insert_string(d_options_TA, FALSE)  &&
            insert_string("G:", FALSE)          &&
            insert_string(filename, FALSE)      &&
            insert_string(",100", FALSE)        &&
            insert_string(d_options_TA, FALSE)  &&
            Return_fn_core()                    )
        {
            draw_screen();
            return(1);
        }
    }

    return(0);
}

/* object (possibly scrap) dragged into main window - insert mostly regardless of type */

static void
main_event_Message_DataLoad_PDMacro(
    char * filename)
{
    if(mystr_set(&d_macro_file[0].textfield, filename))
    {
        do_execfile(d_macro_file[0].textfield);
        str_clr(   &d_macro_file[0].textfield);
    }
}

static void
main_event_Message_DataLoad_PipeDream(
    char * filename,
    FILETYPE_RISC_OS filetype)
{
    STATUS filetype_option = find_filetype_option(filename, filetype); /* checks readability and discriminates PipeDream chart files */

    if(PD4_CHART_CHAR == filetype_option)
    {
        trace_0(TRACE_APP_PD4, "pd chart about to be loaded via text-at field G mechanism");
        draw_insert_filename(filename);
        return;
    }

    (void) riscos_LoadFile(filename, TRUE, filetype_option /*may be 0,Err*/);
}

static void
main_event_Message_DataLoad_others(
    char * filename,
    FILETYPE_RISC_OS filetype)
{
    STATUS filetype_option;

    if(image_cache_can_import(filetype))
    {
        draw_insert_filename(filename);
        return;
    }

    filetype_option = find_filetype_option(filename, filetype); /* check the readability & do the auto-detect here for consistency */

    (void) riscos_LoadFile(filename, TRUE, filetype_option /*may be 0,Err*/);
}

static BOOL
main_event_Message_DataLoad(
    const WimpMessage * const user_message)
{
    char * filename;
    FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkinsert(&filename); /* sets up reply too */

    UNREFERENCED_PARAMETER_InRef_(user_message); /* xferrecv uses last message mechanism */

    reportf("Message_DataLoad(main): file type &%03X, name %u:%s", filetype, strlen32(filename), filename);

    switch(filetype)
    {
    case FILETYPE_DIRECTORY:
    case FILETYPE_APPLICATION:
        reperr(FILE_ERR_ISADIR, filename);
        break;

    case FILETYPE_PD_MACRO:
        main_event_Message_DataLoad_PDMacro(filename);
        break;

    case FILETYPE_PD_CHART:
    case FILETYPE_PIPEDREAM:
        main_event_Message_DataLoad_PipeDream(filename, filetype);
        break;

    default:
        main_event_Message_DataLoad_others(filename, filetype);
        break;
    }

    /* this is mandatory */
    xferrecv_insertfileok();

    return(TRUE);
}

static BOOL
main_event_Message_HelpRequest(
    /*poked*/ WimpMessage * const user_message)
{
    const int gdi_x = user_message->data.help_request.mouse_x;
    const int gdi_y = user_message->data.help_request.mouse_y;
    P_DOCU p_docu = find_document_with_input_focus();
    BOOL insertref = ( (NO_DOCUMENT != p_docu) &&
                       (p_docu->Xxf_inexpression || p_docu->Xxf_inexpression_box || p_docu->Xxf_inexpression_line) );
    char abuffer[256+128/*bit of slop*/];
    U32 prefix_len;
    char * buffer;
    char * alt_msg;
    coord tx = tcoord_x(gdi_x); /* text cell coordinates */
    coord ty = tcoord_y(gdi_y);
    coord coff  = calcoff(tx); /* not _click */
    coord roff  = calroff(ty); /* not _click */
    coord o_roff = roff;
    ROW trow;
    BOOL append_drag_msg = xf_inexpression; /* for THIS window */

    if(drag_type != NO_DRAG_ACTIVE) /* stop pointer and message changing whilst dragging */
        return(TRUE);

    trace_0(TRACE_APP_PD4, "Help request on main window");
    trace_4(TRACE_APP_PD4, "get_slr_for_point: g(%d, %d) t(%d, %d)", gdi_x, gdi_y, tx, ty);

    /* default message */
    xstrkpy(abuffer, elemof32(abuffer), help_main_window);

    prefix_len = strlen(abuffer); /* remember a possible cut place */

    alt_msg = buffer = abuffer + prefix_len;

    if(vertvec_entry_valid(roff))
    {
        P_SCRROW rptr = vertvec_entry(roff);

        if(rptr->flags & PAGE)
        {
            xstrkpy(buffer, elemof32(abuffer) - prefix_len, help_row_is_page_break);
        }
        else
        {
            trow = rptr->rowno;

            if(horzvec_entry_valid(coff)  ||  (coff == OFF_RIGHT))
            {
                COL tcol, scol;
                U8 sbuf[BUF_MAX_REFERENCE];
                U8 abuf[BUF_MAX_REFERENCE];
                const char *msg;

                if( !insertref  &&
                    chkrpb(trow)  &&  chkfsb()  &&  chkpac(trow))
                        xstrkpy(buffer, elemof32(abuffer) - prefix_len, help_row_is_hard_page_break);
                else
                {
                    msg = (insertref)
                               ? help_insert_a_reference_to
                               : help_position_the_caret_in;

                    if(coff == OFF_RIGHT)
                        coff = get_column(tx, trow, 0, FALSE);

                    assert(horzvec_entry_valid(coff));
                    tcol = col_number(coff);
                    trace_2(TRACE_APP_PD4, "in sheet at row #%d, col #%d", trow, tcol);
                    (void) write_ref(abuf, elemof32(abuf), current_docno(), tcol, trow);

                    if(!insertref)
                    {
                        coff = get_column(tx, trow, 0, TRUE);
                        assert(horzvec_entry_valid(coff));
                        scol = col_number(coff);
                        trace_2(TRACE_APP_PD4, "will position at row #%d, col #%d", trow, scol);
                        (void) write_ref(sbuf, elemof32(sbuf), current_docno(), scol, trow);
                    }
                    else
                    {
                        scol = tcol;
                    }

                    (void) xsnprintf(buffer, elemof32(abuffer) - prefix_len,
                            (scol != tcol)
                                ? "%s%s%s" "%s%s"     "%s%s%s"       "%s.|M%s"
                                : "%s%s%s" "%.0s%.0s" "%.0s%.0s%.0s" "%s.|M%s",
                            help_click_select_to, msg, help_cell,
                            /* else miss this set of args */
                            sbuf, ".|M",
                            /* and this set of args */
                            help_click_adjust_to, msg, help_cell,
                            abuf,
                            help_drag_select_to_mark_block);
                }
            }
            else if(IN_ROW_BORDER(coff))
            {
                if(xf_inexpression || xf_inexpression_box || xf_inexpression_line)
                    trace_0(TRACE_APP_PD4, "no action cos editing in THIS sheet");
                else
                    (void) xsnprintf(buffer, elemof32(abuffer) - prefix_len,
                            "%s%s",
                            help_drag_row_border,
                            (o_roff == roff)
                                  ? help_double_row_border
                                  : (const char *) NULLSTR);
            }
            else
                trace_0(TRACE_APP_PD4, "off left/right");
        }
    }
    else
        trace_0(TRACE_APP_PD4, "above/below sheet data");

    if(append_drag_msg && (strlen32p1(abuffer) + strlen32(help_drag_file_to_insert) <= 236))
         xstrkat(abuffer, elemof32(abuffer), help_drag_file_to_insert);

    riscos_send_Message_HelpReply(user_message, (strlen32p1(abuffer) <= 236) ? abuffer : alt_msg);

    return(TRUE);
}

/* help out the iconizer - send him a new message as acknowledgement of his request */

static BOOL
main_event_Message_WindowInfo(
    /*poked*/ WimpMessageExtra * const user_message)
{
    user_message->hdr.size     = sizeof(user_message->hdr) + sizeof(user_message->data.window_info);
    user_message->hdr.your_ref = user_message->hdr.my_ref;
    user_message->hdr.action_code = Wimp_MWindowInfo;

    user_message->data.window_info.reserved_0 = 0;
    (void) strcpy(user_message->data.window_info.sprite, /*"ic_"*/ "dde"); /* dde == PipeDream */
    (void) strcpy(user_message->data.window_info.title, file_leafname(currentfilename()));

    void_WrapOsErrorReporting(wimp_send_message_to_task(Wimp_EUserMessage, (WimpMessage *) user_message, user_message->hdr.sender));

    return(TRUE);
}

static BOOL
main_event_User_Message(
    /*acked*/ WimpMessage * const user_message)
{
    switch(user_message->hdr.action_code)
    {
    case Wimp_MDataSave:
        return(main_event_Message_DataSave(user_message));

    case Wimp_MDataLoad:
        return(main_event_Message_DataLoad(user_message));

    case Wimp_MHelpRequest:
        return(main_event_Message_HelpRequest(user_message));

    case Wimp_MWindowInfo:
        return(main_event_Message_WindowInfo((WimpMessageExtra *) user_message));

    default:
        trace_1(TRACE_APP_PD4, "unprocessed %s message to main window",
                 report_wimp_message(user_message, FALSE));
        return(FALSE);
    }
}

/******************************************************************************
*
* process main window events
*
******************************************************************************/

#define main_event_handler_report(event_code, event_data, handle) \
    riscos_event_handler_report(event_code, event_data, handle, "main")

static BOOL
main_event_handler(
    wimp_eventstr *e,
    void *handle)
{
    const int event_code = (int) e->e;
    WimpPollBlock * const event_data = (WimpPollBlock *) &e->data;

    if(!select_document_using_callback_handle(handle))
    {
        messagef(TEXT("Bad handle ") PTR_XTFMT TEXT(" passed to main event handler"), report_ptr_cast(handle));
        return(FALSE);
    }

    trace_4(TRACE_APP_PD4, TEXT("main_event_handler: event %s, dhan ") PTR_XTFMT TEXT(" window %d, document %d"),
             report_wimp_event(event_code, event_data), report_ptr_cast(handle), (int) main_window_handle, current_docno());

    switch(event_code)
    {
    case Wimp_ERedrawWindow:
        /*main_event_handler_report(event_code, event_data, handle);*/
        return(main_event_Redraw_Window_Request(&event_data->redraw_window_request));

    case Wimp_EOpenWindow:  /* main window always opened as a pane on top of rear window */
    case Wimp_ECloseWindow: /* main window has no close box. rear window has the close box */
        /*main_event_handler_report(event_code, event_data, handle);*/
        return(TRUE); /*not default: */

    case Wimp_EPointerLeavingWindow:
    case Wimp_EPointerEnteringWindow:
        /*main_event_handler_report(event_code, event_data, handle);*/
        return(TRUE); /*not default: */

    case Wimp_EMouseClick:
        /*main_event_handler_report(event_code, event_data, handle);*/
        return(main_event_Mouse_Click(&event_data->mouse_click));

    case Wimp_EKeyPressed:
        /*main_event_handler_report(event_code, event_data, handle);*/
        return(main_event_Key_Pressed(&event_data->key_pressed));

    case Wimp_EScrollRequest:
        main_event_handler_report(event_code, event_data, handle);
        return(main_event_Scroll_Request(&event_data->scroll_request));

    case Wimp_ELoseCaret:
        /*main_event_handler_report(event_code, event_data, handle);*/
        return(main_event_Lose_Caret(e));

    case Wimp_EGainCaret:
        /*main_event_handler_report(event_code, event_data, handle);*/
        return(main_event_Gain_Caret(e));

    case Wimp_EUserMessage:
    case Wimp_EUserMessageRecorded:
        /*main_event_handler_report(event_code, event_data, handle);*/
        return(main_event_User_Message(&event_data->user_message));

    default:
        /*main_event_handler_report(event_code, event_data, handle);*/
        trace_1(TRACE_APP_PD4, "unprocessed wimp event to main window: %s",
                report_wimp_event(event_code, event_data));
        return(FALSE);
    }
}

/*
exported functions
*/

/******************************************************************************
*
* clean up a string
*
******************************************************************************/

extern char *
riscos_cleanupstring(
    char *str)
{
    char *a = str;

    while(*a++ >= 0x20)
        ;

    a[-1] = CH_NULL;

    return(str);
}

/******************************************************************************
*
* if window not already in existence
* in this domain then make a new window
*
******************************************************************************/

extern BOOL
riscos_create_document_window(void)
{
    DOCNO docno = current_docno();
    os_error * e;

    trace_0(TRACE_APP_PD4, "riscos_create_document_window()");

    if(rear_window_handle == HOST_WND_NONE) /* a window needs creating? */
    {
        rear_window_template = template_copy_new(template_find_new(rear_window_name));

        if(NULL == rear_window_template)
            return(reperr_null(status_nomem()));

        riscos_settitlebar(currentfilename()); /* set the buffer content up */

        ((WimpWindowWithBitset *) (current_p_docu->Xrear_window_template))->title_data.it.buffer = current_p_docu->Xwindow_title;
        ((WimpWindowWithBitset *) (current_p_docu->Xrear_window_template))->title_data.it.buffer_size = BUF_WINDOW_TITLE_LEN; /* includes terminator */

        if(NULL != (e =
            winx_create_window(rear_window_template, &rear_window_handle,
                               rear_event_handler, (void *)(uintptr_t)docno)))
            return(reperr_kernel_oserror(e));

        /* Bump the y coordinate for the next window to be created */
        /* Try not to overlap the icon bar */
        riscos_setnextwindowpos(readval_S32(&main_window_definition->box.y1));

        /* get extent and scroll offsets set correctly on initial open */
        out_alteredstate = TRUE;
    }

    if(colh_window_handle == HOST_WND_NONE) /* a window needs creating? */
    {
        colh_window_template = template_copy_new(template_find_new(colh_window_name));

        if(NULL == colh_window_template)
            return(reperr_null(status_nomem()));

        if(NULL != (e =
            winx_create_window(colh_window_template, &colh_window_handle,
                               colh_event_handler, (void *)(uintptr_t)docno)))
            return(reperr_kernel_oserror(e));

        colh_position_icons();      /*>>>errors???*/
    }

    if(main_window_handle == HOST_WND_NONE) /* a window needs creating? */
    {
        main_window_template = template_copy_new(template_find_new(main_window_name));

        if(NULL == main_window_template)
            return(reperr_null(status_nomem()));

        if(NULL != (e =
            winx_create_window(main_window_template, &main_window_handle,
                               main_event_handler, (void *)(uintptr_t)docno)))
            return(reperr_kernel_oserror(e));

        riscmenu_attachmenutree(main_window_handle);
        riscmenu_attachmenutree(colh_window_handle);

        /* Create function paster menu */
        function__event_menu_maker();

        event_attachmenumaker_to_w_i(colh_window_handle,
                                     COLH_FUNCTION_SELECTOR,
                                     function__event_menu_filler,
                                     function__event_menu_proc,
                                     (void *)(uintptr_t)docno);

        /* now window created at default size, initialise screen bits */
        if(!screen_initialise())
            return(FALSE);
    }

    return(TRUE);
}

/******************************************************************************
*
* destroy this domain's document windows
*
******************************************************************************/

static inline void
riscos_template_copy_dispose(
    _Inout_     void ** ppww)
{
    template_copy_dispose((WimpWindowWithBitset **) ppww);
}

extern void
riscos_destroy_document_window(void)
{
    trace_0(TRACE_APP_PD4, "riscos_destroy_document_window()");

    if(rear_window_handle != HOST_WND_NONE)
    {
        /* deregister procedures for the rear window */

        riscos_window_dispose(&rear_window_handle);

        riscos_template_copy_dispose(&rear_window_template);
    }

    if(colh_window_handle != HOST_WND_NONE)
    {
        /* deregister procedures for the colh window */
        riscmenu_detachmenutree(colh_window_handle);

        riscos_window_dispose(&colh_window_handle);

        riscos_template_copy_dispose(&colh_window_template);

        /*>>>should probably give caret away, if this window has it */
    }

    if(main_window_handle != HOST_WND_NONE)
    {
        /* pretty permanent caret loss */
        if(main_window_handle == caret_window_handle)
            caret_window_handle = HOST_WND_NONE;
        if(main_window_handle == caret_stolen_from_window_handle)
            caret_stolen_from_window_handle = HOST_WND_NONE;

        /* deregister procedures for the main window */
        riscmenu_detachmenutree(main_window_handle);

        riscos_window_dispose(&main_window_handle);

        riscos_template_copy_dispose(&main_window_template);
    }
}

extern void
riscos_window_dispose(
    _Inout_     HOST_WND * const p_window_handle)
{
    if(HOST_WND_NONE == *p_window_handle)
        return;

    winx_delete_window(p_window_handle);

    *p_window_handle = HOST_WND_NONE;
}

/******************************************************************************
*
* RISC OS module domain finalisation
*
******************************************************************************/

extern void
riscos_finalise(void)
{
    trace_0(TRACE_APP_PD4, "riscos_finalise()");

    riscos_destroy_document_window();
}

/******************************************************************************
*
* RISC OS module final finalisation
*
******************************************************************************/

extern void
riscos_finalise_once(void)
{
    trace_0(TRACE_APP_PD4, "riscos_finalise_once()");

    if(riscos__initialised)
    {
        riscos__initialised = FALSE;

        #ifdef HAS_FUNNY_CHARACTERS_FOR_WRCH
        wrch_undefinefunnies();
        #endif
    }
}

/******************************************************************************
*
* bring document window to the front
*
******************************************************************************/

extern void
riscos_front_document_window(
    BOOL immediate)
{
    trace_1(TRACE_APP_PD4, "riscos_front_document_window(immediate = %s)", report_boolstring(immediate));

    if(main_window_handle == HOST_WND_NONE)
        return;

    xf_front_document_window = FALSE;                    /* as immediate event raising calls draw_screen() */
    winx_send_front_window_request(rear_window_handle, immediate);
}

extern void
riscos_front_document_window_atbox(
    BOOL immediate)
{
    trace_1(TRACE_APP_PD4, "riscos_front_document_window_atbox(immediate = %s)", report_boolstring(immediate));

    if(main_window_handle == HOST_WND_NONE)
        return;

    xf_front_document_window = FALSE;                    /* as immediate event raising calls draw_screen() */
    winx_send_front_window_request_at(rear_window_handle, immediate, (const BBox *) &open_box);
}

extern BOOL
riscos_adjust_clicked(void)
{
    return(winx_adjustclicked());
}

/******************************************************************************
*
* RISC OS module domain initialisation
*
******************************************************************************/

extern BOOL
riscos_initialise(void)
{
    BOOL ok;

    ok = riscos_create_document_window();

    return(ok);
}

/******************************************************************************
*
* initialise RISC OS module
*
******************************************************************************/

#if 0
#define MIN_X (8 * normal_charwidth)
#define MIN_Y (5 * (normal_charheight + 2*4))
#endif

#define WTEMPLATES_PREFIX "WTemplates."

extern void
riscos_initialise_once(void)
{
    trace_0(TRACE_APP_PD4, "riscos_initialise_once()");

    resspr_init();

    /* register callproc for actions before exiting to Wimp_Poll */
    wimptx_atexit(actions_before_exiting);

    /* no point having more than one of this app loaded */
    g_kill_duplicates = 1;

    /* SKS 25oct96 allow templates to go out */
    template_require(0, WTEMPLATES_PREFIX "tem_risc");
    template_require(1, WTEMPLATES_PREFIX "tem_main");
    template_require(2, WTEMPLATES_PREFIX "tem_cht");
    template_require(3, WTEMPLATES_PREFIX "tem_bc");
    template_require(4, WTEMPLATES_PREFIX "tem_el");
    template_require(5, WTEMPLATES_PREFIX "tem_f");
    template_require(6, WTEMPLATES_PREFIX "tem_p");
    template_require(7, WTEMPLATES_PREFIX "tem_s");
    template_init(); /* NB before dbox_init() */
    dbox_init();

    /* now get a handle onto the definition after the dbox is loaded */

    /* now keep main_window_definition pointing to the back window for all time */

    main_window_definition = (wimp_wind *) template_syshandle_unaligned(rear_window_name);

    main_window_initial_y1     = (int) readval_S32(&main_window_definition->box.y1);
    main_window_default_height = main_window_initial_y1 -
                                 (int) readval_S32(&main_window_definition->box.y0);

#if 0 /* can't see the point of this ... */
    {
    static const char fontselector_dboxname[] = "FontSelect";
    wimp_wind * fontselector_definition_ua = (wimp_wind *) template_syshandle_unaligned(fontselector_dboxname);
    wimp_icon * fontselector_firsticon_ua  = (wimp_icon *) (fontselector_definition_ua + 1);

    writeval_S32(&fontselector_firsticon_ua[FONT_LEADING].flags,
     readval_S32(&fontselector_firsticon_ua[FONT_LEADING].flags)      | wimp_INOSELECT);
    writeval_S32(&fontselector_firsticon_ua[FONT_LEADING_DOWN].flags,
     readval_S32(&fontselector_firsticon_ua[FONT_LEADING_DOWN].flags) | wimp_INOSELECT);
    writeval_S32(&fontselector_firsticon_ua[FONT_LEADING_UP].flags,
     readval_S32(&fontselector_firsticon_ua[FONT_LEADING_UP].flags)   | wimp_INOSELECT);
    writeval_S32(&fontselector_firsticon_ua[FONT_LEADING_AUTO].flags,
     readval_S32(&fontselector_firsticon_ua[FONT_LEADING_AUTO].flags) | wimp_INOSELECT);
    }
#endif

    iconbar_initialise(product_id());

    /* Direct miscellaneous unknown (and icon bar)
     * events to our default event handler
    */
    winx_register_new_event_handler(win_ICONBARLOAD,
                                    default_event_handler,
                                    NULL);
    win_claim_unknown_events(win_ICONBARLOAD);

    /* initialise riscmenu */
    riscmenu_initialise_once();

    /* initialise riscdraw */
    cache_mode_variables(); /* includes palette data */

    riscprint_set_printer_data();

    riscos__initialised = TRUE;
}

/******************************************************************************
*
* invalidate all of main window & colh window (rear window is fully covered)
*
******************************************************************************/

extern void
riscos_invalidate_document_window(void)
{
    if(main_window_handle != HOST_WND_NONE)
    {
        BBox redraw_area;

        redraw_area.xmin = -0x07FFFFFF;
        redraw_area.ymin = -0x07FFFFFF;
        redraw_area.xmax = +0x07FFFFFF;
        redraw_area.ymax = +0x07FFFFFF;

        void_WrapOsErrorReporting(
            tbl_wimp_force_redraw(main_window_handle,
                                  redraw_area.xmin, redraw_area.ymin,
                                  redraw_area.xmax, redraw_area.ymax));
    }

    if(colh_window_handle != HOST_WND_NONE)
    {
        BBox redraw_area;

        colh_colour_icons();                    /* ensure colours of wimp maintained icons are correct */

        redraw_area.xmin = -0x07FFFFFF;
        redraw_area.ymin = -0x07FFFFFF;
        redraw_area.xmax = +0x07FFFFFF;
        redraw_area.ymax = +0x07FFFFFF;

        void_WrapOsErrorReporting(
            tbl_wimp_force_redraw(colh_window_handle, /* will force icons maintained by us to be redrawn */
                                  redraw_area.xmin, redraw_area.ymin,
                                  redraw_area.xmax, redraw_area.ymax));
    }
}

/******************************************************************************
*
* derive a filename to use given the current state
*
******************************************************************************/

extern PCTSTR
riscos_obtainfilename(
    _In_opt_z_  PCTSTR filename)
{
    if(filename == NULL)
        return(TEXT("<Untitled>"));

    return(filename);
}

/******************************************************************************
*
*  read info from a file into a fileinfo block
*
******************************************************************************/

extern BOOL
riscos_readfileinfo(
    RISCOS_FILEINFO * const rip /*out*/,
    const char * name)
{
    _kernel_osfile_block fileblk;
    int res;

    res = _kernel_osfile(OSFile_ReadNoPath, name, &fileblk);

    if(res == _kernel_ERROR)
        void_WrapOsErrorReporting(_kernel_last_oserror());

    if(res != OSFile_ObjectType_File)
    {   /* includes error case */
        riscos_readtime(rip);
        riscos_settype(rip, FILETYPE_PIPEDREAM);
        rip->length = 0;
    }
    else
    {
        rip->exec   = fileblk.exec;
        rip->load   = fileblk.load;
        rip->length = fileblk.start;
    }

    reportf("riscos_readfileinfo(%u:%s): type=%d, length=%u, load=0x%08X, exec=0x%08X",
            strlen(name), name, res, (size_t) rip->length, rip->load, rip->exec);

    return(res == OSFile_ObjectType_File);
}

/******************************************************************************
*
* read the current time from OS into a fileinfo block
*
******************************************************************************/

extern void
riscos_readtime(
    RISCOS_FILEINFO * const rip /*inout*/)
{
    rip->exec = 3;           /* cs since 1900 */
    (void) _kernel_osword(14, (int *) rip); /* sets exec and LSB of load */
}

/******************************************************************************
*
*  start windows from one up again
*
******************************************************************************/

extern void
riscos_resetwindowpos(void)
{
    S32 y1;

    y1 = readval_S32(&main_window_definition->box.y1);

    if(y1 != main_window_initial_y1)
        riscos_setdefwindowpos(y1 + main_window_y_bump);
}

/******************************************************************************
*
* send a help reply
*
******************************************************************************/

extern void
riscos_send_Message_HelpReply(
    /*acked*/ WimpMessage * const user_message,
    const char * msg)
{
    S32 length = strlen32p1(msg);

    if(length > sizeof32(user_message->data.help_reply.text))
        length = sizeof32(user_message->data.help_reply.text); /* xstrkpy() will truncate */

    if(user_message->data.help_request.icon_handle >= -1)
    {
        S32 size = offsetof(WimpMessage, data.help_reply.text) + length;

        trace_2(TRACE_APP_PD4, "help_reply is %d long, %s", size, msg);

        user_message->hdr.size        = size;
        user_message->hdr.your_ref    = user_message->hdr.my_ref;
        user_message->hdr.action_code = Wimp_MHelpReply;

        xstrkpy(user_message->data.help_reply.text, sizeof32(user_message->data.help_reply.text), msg);

        void_WrapOsErrorReporting(wimp_send_message_to_task(Wimp_EUserMessage, user_message, user_message->hdr.sender));
    }
    else
        trace_0(TRACE_APP_PD4, "no reply for system icons");
}

/******************************************************************************
*
* start windows at given y1
*
******************************************************************************/

extern void
riscos_setdefwindowpos(
    S32 new_y1)
{
    S32 new_y0 = new_y1 - main_window_default_height;

    writeval_S32(&main_window_definition->box.y0, new_y0);
    writeval_S32(&main_window_definition->box.y1, new_y1);
}

extern void
riscos_setinitwindowpos(
    S32 init_y1)
{
    /* first open determines all other opens says SKS */
    main_window_initial_y1 = init_y1;
}

extern void
riscos_setnextwindowpos(
    S32 this_y1)
{
    this_y1 -= main_window_y_bump;

    if((this_y1 - main_window_default_height) <= (iconbar_height + wimptx_hscroll_height()))
        this_y1 = main_window_initial_y1;

    riscos_setdefwindowpos(this_y1);
}

/******************************************************************************
*
* Set title bar of document window given current state
*
******************************************************************************/

extern void
riscos_settitlebar(
    const char *filename)
{
    PCTSTR documentname = riscos_obtainfilename(filename);

    trace_2(TRACE_APP_PD4, "riscos_settitlebar(%s): rear_window_handle %d", documentname, rear_window_handle);

#ifndef TITLE_SPACE
#define TITLE_SPACE " "
#endif

    /* carefully copy information into the title string */
    xstrkpy(current_p_docu->Xwindow_title, BUF_WINDOW_TITLE_LEN, product_ui_id());
    xstrkat(current_p_docu->Xwindow_title, BUF_WINDOW_TITLE_LEN, ":" TITLE_SPACE);
    xstrkat(current_p_docu->Xwindow_title, BUF_WINDOW_TITLE_LEN, documentname);
    if(xf_filealtered)
        xstrkat(current_p_docu->Xwindow_title, BUF_WINDOW_TITLE_LEN, TITLE_SPACE "*");
    trace_1(TRACE_APP_PD4, "poked main document title to be '%s'", current_p_docu->Xwindow_title);

    if(rear_window_handle != HOST_WND_NONE)
        winx_changedtitle(rear_window_handle);
}

/******************************************************************************
*
* set the type of a fileinfo
*
******************************************************************************/

extern void
riscos_settype(
    RISCOS_FILEINFO * const rip /*inout*/,
    int filetype)
{
    rip->load = (rip->load & 0x000000FF) | (filetype << 8) | 0xFFF00000;
}

/******************************************************************************
*
* invalidate part of main window and call application back to redraw it
* NB. this takes offsets from xorg/yorg
*
******************************************************************************/

extern void
riscos_updatearea(
    RISCOS_REDRAWPROC redrawproc,
    _HwndRef_   HOST_WND window_handle,
    int x0,
    int y0,
    int x1,
    int y1)
{
    WimpUpdateAndRedrawWindowBlock update_and_redraw_window_block;
    BOOL more;

    trace_6(TRACE_APP_PD4, "riscos_updatearea(%s, %d, %d, %d, %d, %d)",
             report_procedure_name(report_proc_cast(redrawproc)), window_handle, x0, y0, x1, y1);

    /* RISC OS graphics primitives all need coordinates limited to s16 */
    x0 = MAX(x0, SHRT_MIN);
    y0 = MAX(y0, SHRT_MIN);
    x1 = MIN(x1, SHRT_MAX);
    y1 = MIN(y1, SHRT_MAX);

    update_and_redraw_window_block.update.window_handle = window_handle;
    update_and_redraw_window_block.update.update_area.xmin = x0;
    update_and_redraw_window_block.update.update_area.ymin = y0;
    update_and_redraw_window_block.update.update_area.xmax = x1;
    update_and_redraw_window_block.update.update_area.ymax = y1;

    /* wimp errors in update are fatal */
    if(NULL != WrapOsErrorReporting(tbl_wimp_update_window(&update_and_redraw_window_block.redraw, &more)))
        more = FALSE;

    while(more)
    {
        killcolourcache();

        graphics_window = * ((PC_GDI_BOX) &update_and_redraw_window_block.redraw.redraw_area);

        #if !defined(ALWAYS_PAINT)
        paint_is_update = TRUE;
        #endif

        /* we are updating area */
        redrawproc((const RISCOS_RedrawWindowBlock *) &update_and_redraw_window_block.redraw);

        if(NULL != WrapOsErrorReporting(tbl_wimp_get_rectangle(&update_and_redraw_window_block.redraw, &more)))
            more = FALSE;
    }
}

/******************************************************************************
*
* write a fileinfo block data onto a file
*
******************************************************************************/

extern void
riscos_writefileinfo(
    const RISCOS_FILEINFO * const rip,
    const char * name)
{
    _kernel_osfile_block fileblk;
    int res;

    reportf("riscos_writefileinfo(%u:%s): load=0x%08X, exec=0x%08X",
            strlen32(name), name, rip->load, rip->exec);

    fileblk.load = rip->load;
    res = _kernel_osfile(OSFile_WriteLoad, name, &fileblk);

    if(res != _kernel_ERROR)
    {
        fileblk.exec = rip->exec;
        res = _kernel_osfile(OSFile_WriteExec, name, &fileblk);
    }

    if(res == _kernel_ERROR)
        void_WrapOsErrorReporting(_kernel_last_oserror());
}

extern void
filer_opendir(
    _In_z_      PCTSTR filename)
{
    PCTSTR leafname = file_leafname(filename);

    if(leafname != filename)
    {
        TCHARZ cmdbuffer[BUF_MAX_PATHSTRING];

        (void) xsnprintf(cmdbuffer, elemof32(cmdbuffer), "Filer_OpenDir %.*s", (leafname - filename) - 1, filename);

        /* which will pop up ca. 0.5 seconds later ... */
        (void) _kernel_oscli(cmdbuffer);
    }
}

extern void
filer_launch(
    _In_z_      PCTSTR filename)
{
    TCHARZ cmdbuffer[BUF_MAX_PATHSTRING];

    (void) xsnprintf(cmdbuffer, elemof32(cmdbuffer), "Filer_Run %s", filename);

    (void) _kernel_oscli(cmdbuffer);
}

/*
Re-written replacements for RISC_OSLib:akbd.c functions
*/

static __inline int
akbd__poll(
    int x)
{
    int r = _kernel_osbyte(0x81, x, 255);
    return((r & 0x0000FFFF) == 0x0000FFFF);     /* X and Y both 255? */
}

extern int
akbd_pollsh(void)
{
    return(akbd__poll(-1));
}

extern int
akbd_pollctl(void)
{
    return(akbd__poll(-2));
}

/* tboxlibs compatible shims a la WimpLib */

_kernel_oserror *
tbl_wimp_create_window(WimpWindow *defn, int *handle)
{
    _kernel_oserror * e;
    _kernel_swi_regs rs;

    rs.r[1] = (int) defn;

    e = _kernel_swi(Wimp_CreateWindow, &rs, &rs);

    *handle = e ? 0 : rs.r[0];

    return(e);
}

_kernel_oserror *
tbl_wimp_create_icon(
    int priority,
    WimpCreateIconBlock *defn,
    int *handle)
{
    _kernel_oserror * e;
    _kernel_swi_regs rs;

    rs.r[0] = (int) priority;
    rs.r[1] = (int) defn;

    e = _kernel_swi(Wimp_CreateIcon, &rs, &rs);

    *handle = e ? 0 : rs.r[0];

    return(e);
}

_kernel_oserror *
tbl_wimp_delete_window(WimpDeleteWindowBlock *block)
{
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    return(_kernel_swi(Wimp_DeleteWindow, &rs, &rs));
}

_kernel_oserror *
tbl_wimp_open_window(WimpOpenWindowBlock *show)
{
    _kernel_swi_regs rs;

    rs.r[1] = (int) show;

    return(_kernel_swi(Wimp_OpenWindow, &rs, &rs));
}

_kernel_oserror *
tbl_wimp_redraw_window(WimpRedrawWindowBlock *block, int *more)
{
    _kernel_oserror * e;
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    e = _kernel_swi(Wimp_RedrawWindow, &rs, &rs);

    *more = e ? 0 : rs.r[0];

    return(e);
}

_kernel_oserror *
tbl_wimp_update_window(WimpRedrawWindowBlock *block, int *more)
{
    return(wimp_update_wind((wimp_redrawstr *) block, more));
}

_kernel_oserror *
tbl_wimp_get_rectangle(WimpRedrawWindowBlock *block, int *more)
{
    return(wimp_get_rectangle((wimp_redrawstr *) block, more));
}

_kernel_oserror *
tbl_wimp_get_window_state(WimpGetWindowStateBlock *state)
{
    _kernel_swi_regs rs;

    rs.r[1] = (int) state;

    return(_kernel_swi(Wimp_GetWindowState, &rs, &rs));
}

_kernel_oserror *
tbl_wimp_set_icon_state(WimpSetIconStateBlock *block)
{
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    return(_kernel_swi(Wimp_SetIconState, &rs, &rs));
}

_kernel_oserror *
tbl_wimp_get_icon_state(WimpGetIconStateBlock *block)
{
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    return(_kernel_swi(Wimp_GetIconState, &rs, &rs));
}

_kernel_oserror *
tbl_wimp_force_redraw(
    int window_handle,
    int xmin,
    int ymin,
    int xmax,
    int ymax)
{
    WimpForceRedrawBlock force_redraw_block;

    force_redraw_block.window_handle = window_handle;
    force_redraw_block.redraw_area.xmin = xmin;
    force_redraw_block.redraw_area.ymin = ymin;
    force_redraw_block.redraw_area.xmax = xmax;
    force_redraw_block.redraw_area.ymax = ymax;

    return(_kernel_swi(Wimp_ForceRedraw, (_kernel_swi_regs *) &force_redraw_block, (_kernel_swi_regs *) &force_redraw_block));
}

_kernel_oserror *
tbl_wimp_set_caret_position(
    int window_handle,
    int icon_handle,
    int xoffset,
    int yoffset,
    int height,
    int index)
{
    _kernel_swi_regs rs;

    rs.r[0] = window_handle;
    rs.r[1] = icon_handle;
    rs.r[2] = xoffset;
    rs.r[3] = yoffset;
    rs.r[4] = height;
    rs.r[5] = index;

    return(_kernel_swi(Wimp_SetCaretPosition, &rs, &rs));
}

_kernel_oserror *
tbl_wimp_get_caret_position(WimpGetCaretPositionBlock *block)
{
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    return(_kernel_swi(Wimp_GetCaretPosition, &rs, &rs));
}

_kernel_oserror *
tbl_wimp_set_extent(int window_handle, BBox *area)
{
    _kernel_swi_regs rs;

    rs.r[0] = window_handle;
    rs.r[1] = (int) area;

    return(_kernel_swi(Wimp_SetExtent, &rs, &rs));
}

_kernel_oserror *
tbl_wimp_plot_icon(WimpPlotIconBlock *block)
{
    _kernel_swi_regs rs;

    rs.r[1] = (int) block;

    return(_kernel_swi(Wimp_PlotIcon, &rs, &rs));
}

_kernel_oserror *
tbl_wimp_block_copy(
    int handle,
    int sxmin,
    int symin,
    int sxmax,
    int symax,
    int dxmin,
    int dymin)
{
    _kernel_swi_regs rs;

    rs.r[0] = handle;
    rs.r[1] = sxmin;
    rs.r[2] = symin;
    rs.r[3] = sxmax;
    rs.r[4] = symax;
    rs.r[5] = dxmin;
    rs.r[6] = dymin;

    return(_kernel_swi(Wimp_BlockCopy, &rs, &rs));
}

/* end of riscos.c */
