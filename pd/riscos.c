/* riscos.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1989-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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

#ifndef __resspr_h
#include "resspr.h"
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

static wimp_openstr saved_window_pos;

/* ------------------------ file import/export --------------------------- */

/* ask for scrap file load: all errors have been reported locally */

static void
scraptransfer_file(
    const wimp_msgdatasave * ms,
    BOOL iconbar)
{
    S32 size;
    FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkimport(&size);

    switch(filetype)
        {
        case FILETYPE_DIRECTORY:
        case FILETYPE_APPLICATION:
            reperr(create_error(FILE_ERR_ISADIR), ms->leaf);
            break;

        case FILETYPE_PIPEDREAM:
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
                    break;
                    }

                /* fault these as we can't store them anywhere safe relative
                 * to the iconbar or relative to this unsaved file
                */
                if(iconbar)
                    reperr_null(create_error(GR_CHART_ERR_DRAWFILES_MUST_BE_SAVED));
                else if(!file_is_rooted(currentfilename))
                    reperr_null(ERR_CHARTIMPORTNEEDSSAVEDDOC);
                else
                    {
                    U32 prefix_len;
                    S32 num = 0;

                    file_get_prefix(myname, elemof32(myname), currentfilename);

                    prefix_len = strlen32(myname);

                    do  {
                        if(++num > 99) /* only two digits in printf string */
                            break;

                        /* invent a new leafname and stick it on the end */
                        (void) xsnprintf(myname + prefix_len, elemof32(myname) - prefix_len,
                                string_lookup(GR_CHART_MSG_DEFAULT_CHARTINNAMEZD),
                                file_leafname(currentfilename),
                                num);

                        /* see if there's any file of this name already */
                        }
                    while(file_is_file(myname));

                    xferrecv_import_via_file(myname);
                    }
                }
            else
                xferrecv_import_via_file(NULL); /* can't manage ram load */
            }
            break;

        default:
            if(gr_cache_can_import(filetype))
                /* fault these as we can't store them anywhere safe */
                reperr_null(create_error(GR_CHART_ERR_DRAWFILES_MUST_BE_SAVED));
            else
                xferrecv_import_via_file(NULL); /* can't manage ram load */
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

        zero_struct(load_file_options);
        load_file_options.document_name = filename;
        load_file_options.temp_file = temp_file;
        load_file_options.inserting = inserting;
        load_file_options.filetype_option = filetype_option;
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

    filetype_option = find_filetype_option(filename);

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
    S32 res;
    char mess[256];

    dbox_note_position_on_completion(TRUE);

    (void) xsnprintf(mess, elemof32(mess),
            (nmodified == 1)
                ? Zd_file_edited_in_Zs_are_you_sure_STR
                : Zd_files_edited_in_Zs_are_you_sure_STR,
            nmodified,
            product_ui_id());

    res = riscdialog_query_SDC(mess);

    /* Discard   -> ok, quit application (also known as 'Discard') */
    /* Save      -> save files, then quit application (aka 'Save') */
    /* Cancel    -> abandon closedown */

    if(res == riscdialog_query_SDC_SAVE)
        {
        /* loop over all documents in sequence trying to save them */

        for(p_docu = first_document(); NO_DOCUMENT != p_docu; p_docu = next_document(p_docu))
            {
            select_document(p_docu);

            dbox_note_position_on_completion(TRUE);

            /* If punter (or program) cancels any save, he means abort the shutdown */
            if(riscdialog_query_save_existing() == riscdialog_query_CANCEL)
                {
                res = riscdialog_query_CANCEL;
                break;
                }
            }
        }

    dbox_noted_position_trash();

    /* if not aborted then all modified documents either saved or ignored - delete all documents (must be at least one) */

    if((res == riscdialog_query_SDC_SAVE) || (res == riscdialog_query_SDC_DISCARD))
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

    select_document_using_docno(old_docno);
    return(0);
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
#define            main_window_y_bump           (wimpt_title_height() + 16)

static const char  fontselector_dboxname[] = "FontSelect";

/*
can we print a file of this filetype?
*/

static BOOL
pd_can_print(
    S32 rft)
{
    switch(rft)
        {
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
    S32 rft)
{
    switch(rft)
        {
        case FILETYPE_PIPEDREAM:
        case FILETYPE_PDMACRO:
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

static void
iconbar_event_EBUT_BLEFT(void)
{
    if(host_ctrl_pressed())
    { /*EMPTY*/ } /* reserved */
    else if(host_shift_pressed())
    {
        TCHARZ filename[BUF_MAX_PATHSTRING];

        /* open the user's Choices directory viewer, from where they can find CmdFiles, Templates etc. */
        tstr_xstrkpy(filename, elemof32(filename), TEXT("<Choices$Write>") FILE_DIR_SEP_STR TEXT("PipeDream") FILE_DIR_SEP_STR);
        tstr_xstrkat(filename, elemof32(filename), TEXT("X"));

        filer_opendir(filename);
    }
    else
        application_process_command(N_LoadTemplate);
}

static void
iconbar_event_EBUT_BRIGHT(void)
{
    if(host_ctrl_pressed())
        { /*EMPTY*/ } /* reserved */
    else if(host_shift_pressed())
        { /*EMPTY*/ } /* reserved */
    else
        application_process_command(N_NewWindow);
}

static void
iconbar_event_EBUT(
    wimp_eventstr *e)
{
    if(e->data.but.m.bbits == wimp_BLEFT)
        iconbar_event_EBUT_BLEFT();
    else if(e->data.but.m.bbits == wimp_BRIGHT)
        iconbar_event_EBUT_BRIGHT();
    else
        trace_0(TRACE_APP_PD4, "wierd button click - ignored");
}

/* create an icon on the icon bar */

static struct
{
    wimp_i i;
    char spritename[1 + 12 + 1];
}
bi;

static BOOL
iconbar_new_sprite(
    const char * spritename)
{
    wimp_icreate i;

    bi.spritename[0] = 'S';
    bi.spritename[1] = '\0';
    xstrkat(bi.spritename, elemof32(bi.spritename), spritename);

    i.w = -1; /* icon bar, right hand side */

    i.i.box.x0 = 0;
    i.i.box.y0 = 0;
    i.i.box.x1 = 68 /* sprite width */;
    i.i.box.y1 = 68 /* sprite height */;

    i.i.flags  = (wimp_iconflags) ( (wimp_IFORECOL * 7) | (wimp_IBACKCOL * 1) |
                                    wimp_INDIRECT | wimp_IFILLED | wimp_IHCENTRE |
                                    (wimp_BCLICKDEBOUNCE * wimp_IBTYPE) |
                                    wimp_ISPRITE );

    i.i.data.indirectsprite.name       = &bi.spritename[1];
    i.i.data.indirectsprite.spritearea = resspr_area();
    i.i.data.indirectsprite.nameisname = TRUE;

    if(wimpt_complain(wimp_create_icon(&i, &bi.i)))
        return(FALSE);

    return(TRUE);
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

    win_register_new_event_handler(win_ICONBAR,
                                   default_event_handler,
                                   NULL);

    /* new icon (sprite/no text) */
    (void) iconbar_new_sprite(iconbar_spritename);
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
check_if_null_events_wanted(wimp_eventstr * e)
{
    BOOL required = FALSE;

    if(e->e != wimp_ENULL)
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
* Tidy up before returning control back to the Window manager
*
******************************************************************************/

static void
actions_before_exiting(wimp_eventstr * e)
{
    /* anyone need background events? */
    check_if_null_events_wanted(e);

    if(wimp_ENULL == e->e)
    {   /* all the things to do once we have settled down */

        /* anyone need end of data sending? */
        send_end_markers();
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
    wimp_msgstr msg;

    if(glp)
        {
        msg.hdr.size   = offsetof(wimp_msgstr, data.pd_dde.type.b)
                       + sizeof(WIMP_MSGPD_DDETYPEB);
        msg.hdr.my_ref = 0;        /* fresh msg */
        msg.hdr.action = Wimp_MPD_DDE;

        msg.data.pd_dde.id            = Wimp_MPD_DDE_SheetClosed;
        msg.data.pd_dde.type.b.handle = glp->ghan;

        wimpt_safe(wimp_sendmessage(wimp_ESEND, &msg, (wimp_t) glp->task));
        }
}

/******************************************************************************
*
* send client the contents of a cell
*
******************************************************************************/

extern void
riscos_sendslotcontents(
    _InoutRef_opt_ P_GRAPHICS_LINK_ENTRY glp,
    S32 xoff,
    S32 yoff)
{
    if(glp)
        {
        ROW row;
        char buffer[LIN_BUFSIZ];
        const char *ptr;
        char *to, ch;
        S32 nbytes;
        WIMP_MSGPD_DDETYPEC_TYPE type;
        wimp_msgstr msg;
        P_CELL tcell;

        glp->datasent = TRUE;

        nbytes = 0;
        type = Wimp_MPD_DDE_typeC_type_Text;
        row = glp->row + yoff;
        ptr = NULL;

        tcell = travel(glp->col + xoff, row);

        if(tcell)
            {
            P_EV_RESULT p_ev_result;

            if(result_extract(tcell, &p_ev_result) == SL_NUMBER)
                {
                switch(p_ev_result->did_num)
                    {
                    case RPN_DAT_WORD8:
                    case RPN_DAT_WORD16:
                    case RPN_DAT_WORD32:
                        msg.data.pd_dde.type.c.content.number = (F64) p_ev_result->arg.integer;
                        goto send_number;

                    case RPN_DAT_REAL:
                        msg.data.pd_dde.type.c.content.number = p_ev_result->arg.fp;
                    send_number:
                        type = Wimp_MPD_DDE_typeC_type_Number;
                        nbytes = sizeof(F64);
                        break;

                    default:
                        break;
                    }
                }

            if(!nbytes)
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

                (void) expand_slot(current_docno(), tcell, row, buffer, LIN_BUFSIZ,
                                   DEFAULT_EXPAND_REFS /*expand_refs*/, TRUE /*expand_ats*/, TRUE /*expand_ctrl*/,
                                   FALSE /*allow_fonty_result*/, TRUE /*cff*/);

                tcell->justify               = t_justify;
                if(tcell->type == SL_NUMBER)
                    tcell->format = t_format;
                d_options_DF      = t_opt_DF;
                d_options_TH      = t_opt_TH;

                /* remove highlights & funny spaces etc from plain non-fonty string */
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
            xstrkpy(msg.data.pd_dde.type.c.content.text, elemof32(msg.data.pd_dde.type.c.content.text), ptr);
            nbytes = strlen32p1(msg.data.pd_dde.type.c.content.text);
            }

        msg.hdr.size   = offsetof(wimp_msgstr, data.pd_dde.type.c.content)
                       + nbytes;
        msg.hdr.my_ref = 0;        /* fresh msg */
        msg.hdr.action = Wimp_MPD_DDE;

        msg.data.pd_dde.id            = Wimp_MPD_DDE_SendSlotContents;
        msg.data.pd_dde.type.c.handle = glp->ghan;
        msg.data.pd_dde.type.c.xoff   = xoff;
        msg.data.pd_dde.type.c.yoff   = yoff;
        msg.data.pd_dde.type.c.type   = type;

        trace_2(TRACE_MODULE_UREF, "sendslotcontents x:%d, y:%d", xoff, yoff);

        wimpt_safe(wimp_sendmessage(wimp_ESENDWANTACK, &msg, (wimp_t) glp->task));
        }
}

extern void
riscos_sendallslots(
    _InoutRef_  P_GRAPHICS_LINK_ENTRY glp)
{
    S32 xoff, yoff;

    xoff = 0;
    do  {
        yoff = 0;
        do  {
            riscos_sendslotcontents(glp, xoff, yoff);
            }
        while(++yoff <= glp->ysize);
        }
    while(++xoff <= glp->xsize);
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
    wimp_msgstr msg;

    if(glp)
        {
        glp->datasent = FALSE;

        msg.hdr.size   = offsetof(wimp_msgstr, data.pd_dde.type.c)
                       + sizeof(WIMP_MSGPD_DDETYPEC);
        msg.hdr.my_ref = 0;        /* fresh msg */
        msg.hdr.action = Wimp_MPD_DDE;

        msg.data.pd_dde.id            = Wimp_MPD_DDE_SendSlotContents;
        msg.data.pd_dde.type.c.handle = glp->ghan;
        msg.data.pd_dde.type.c.type   = Wimp_MPD_DDE_typeC_type_End;

        wimpt_safe(wimp_sendmessage(wimp_ESENDWANTACK, &msg, (wimp_t) glp->task));
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

   while((ch = *p_u8++) != NULLCH)
       if(_kernel_ERROR == _kernel_osbput(ch, os_file_handle))
           return((os_error *) _kernel_last_oserror());

    return(NULL);
}

/******************************************************************************
*
*  event processing for broadcasts
*  and other such events that don't go
*  to a main window handler
*
******************************************************************************/

static void
wimpt_ack_message(
    wimp_msgstr *msg)
{
    wimp_t sender_id = msg->hdr.task;

    trace_0(TRACE_RISCOS_HOST, "acknowledging message: ");

    msg->hdr.your_ref = msg->hdr.my_ref;

    wimpt_safe(wimp_sendmessage(wimp_EACK, msg, sender_id));
}

/******************************************************************************
*
* derived from strncatind, but returns pointer to NULLCH byte at end of a
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

        *a  = '\0';
        *np = n;
        }

    return(a);
}

/******************************************************************************
*
* process message events
*
******************************************************************************/

static BOOL
iconbar_message_DATAOPEN(
    wimp_msgstr *m)
{
    BOOL processed = TRUE;
    char * filename;
    FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkinsert(&filename);
    STATUS filetype_option;

    IGNOREPARM(m); /* xferrecv uses last message mechanism */

    trace_1(TRACE_APP_PD4, "ukprocessor got asked if it can 'run' a file of type &%4.4X", filetype);

    switch(filetype)
        {
        case FILETYPE_PDMACRO:
            /* need this here to stop pause in macro file allowing message bounce
             * thereby allowing Filer to try to invoke another copy of PipeDream
             * to run this macro file ...
            */
            xferrecv_insertfileok();

            reportf("MDATAOPEN: file type &%4.4X, name %u:%s", filetype, strlen32(filename), filename);

            if(mystr_set(&d_macro_file[0].textfield, filename))
                {
                exec_file(d_macro_file[0].textfield);
                str_clr( &d_macro_file[0].textfield);
                }

            break;

        default:
            if(pd_can_run(filetype))
                {
                DOCNO docno;

                xferrecv_insertfileok();

                reportf("MDATAOPEN: file type &%4.4X, name %u:%s", filetype, strlen32(filename), filename);

                filetype_option = find_filetype_option(filename); /* check the readability & do the auto-detect here for consistency */

                if((filetype_option > 0)  &&  (DOCNO_NONE != (docno = find_document_using_wholename(filename))))
                    {
                    front_document_using_docno(docno);
                    }
                else
                    {
                    (void) riscos_LoadFile(filename, FALSE, filetype_option /*may be 0,Err*/);
                    }
                }

            break;
        }

    return(processed);
}

static BOOL
iconbar_message_DATALOAD(
    wimp_msgstr *m)
{
    BOOL processed = TRUE;
    char * filename;
    FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkinsert(&filename);
    STATUS filetype_option;
    DOCNO docno;

    IGNOREPARM(m); /* xferrecv uses last message mechanism */

    reportf("MDATALOAD: file type &%4.4X, name %u:%s", filetype, strlen32(filename), filename);

    switch(filetype)
        {
        case FILETYPE_DIRECTORY:
        case FILETYPE_APPLICATION:
            reperr(create_error(FILE_ERR_ISADIR), filename);
            break;

        case FILETYPE_PIPEDREAM:
            filetype_option = find_filetype_option(filename); /* checks readability and discriminates PipeDream chart files */

            if((filetype_option > 0)  &&  (DOCNO_NONE != (docno = find_document_using_wholename(filename))))
                {
                front_document_using_docno(docno);
                }
            else
                {
                (void) riscos_LoadFile(filename, FALSE, filetype_option /*may be 0,Err*/);
                }

            /* delete NOW if was scrap */
            xferrecv_insertfileok();
            break;

        default:
            if(gr_cache_can_import(filetype))
                {
                trace_0(TRACE_APP_PD4, "ignore Draw file as we can't do anything sensible");
                break;
                }

            trace_0(TRACE_APP_PD4, "loading file as new file");

            if(filetype == FILETYPE_PDMACRO)
                {
                if((filetype_option = file_readable(filename)) > 0)
                    filetype_option = 'T';
                }
            else if(filetype == FILETYPE_CSV)
                {
                if((filetype_option = file_readable(filename)) > 0)
                    filetype_option = CSV_CHAR;
                }
            else
                filetype_option = find_filetype_option(filename); /* check the readability & do the auto-detect here for consistency */

            if((filetype_option > 0)  &&  (DOCNO_NONE != (docno = find_document_using_wholename(filename))))
                {
                front_document_using_docno(docno);
                }
            else
                {
                (void) riscos_LoadFile(filename, FALSE, filetype_option /*may be 0,Err*/);
                }

            /* delete NOW if was scrap */
            xferrecv_insertfileok();
            break;
        }

    return(processed);
}

static BOOL
iconbar_PREQUIT(
    wimp_msgstr *m)
{
    BOOL processed = TRUE;
    const int size = m->hdr.size;
    const wimp_t task = m->hdr.task; /* caller's task id */
    int flags = 0;
    int count;

    /* flags word only present if RISC OS 3 or better */
    if(size >= 24)
       flags = m->data.prequitrequest.flags & wimp_MPREQUIT_flags_killthistask;

    mergebuf_all();            /* ensure modified buffers to docs */

    count = documents_modified();

    /* if any modified, it gets hard */
    if(count)
        {
        /* First, acknowledge the message to stop it going any further */
        wimpt_ack_message(m);

        /* And then tell the user. */
        if(riscos_quit_okayed(count))
            {
            wimp_eventdata ee;

            if(flags) /* only RISC OS 3 */
                exit(EXIT_SUCCESS);

            /* Start up the closedown sequence again if OK and all tasks are to die. (RISC OS 3) */
            /* We assume that the sender is the Task Manager,
             * so that Ctrl-Shf-F12 is the closedown key sequence.
            */
            wimpt_safe(wimp_get_caret_pos(&ee.key.c));
            ee.key.chcode = akbd_Sh + akbd_Ctl + akbd_Fn12;
            wimpt_safe(wimp_sendmessage(wimp_EKEY, (wimp_msgstr *) &ee, task));
            }
        }

    return(processed);
}

static BOOL
iconbar_SAVEDESK(
    wimp_msgstr *m)
{
    BOOL processed = TRUE;
    int os_file_handle = m->data.savedesk.filehandle;
    os_error * e = NULL;
    U8 var_name[BUF_MAX_PATHSTRING];
    U8 main_dir[BUF_MAX_PATHSTRING];
    U8 user_path[BUF_MAX_PATHSTRING];
    P_U8 p_main_app = main_dir;

    main_dir[0] = NULLCH;
    make_var_name(var_name, elemof32(var_name), "$Dir");
    (void) _kernel_getenv(var_name, main_dir, elemof32(main_dir));

    user_path[0] = NULLCH;
    make_var_name(var_name, elemof32(var_name), "$UserPath");
    (void) _kernel_getenv(var_name, user_path, elemof32(user_path));
    /* Ignore PRM guideline about caching at startup; final seen one is most relevant */

    /* write out commands to desktop save file that will restart app */
    if(user_path[0] != NULLCH)
        {
        P_U8 p_u8;

        /* need to boot main app first */
        if((e = _kernel_fwrite0(os_file_handle, "Filer_Boot ")) == NULL)
        if((e = _kernel_fwrite0(os_file_handle, p_main_app)) == NULL)
            e = _kernel_fwrite0(os_file_handle, "\x0A");

        p_u8 = user_path + strlen(user_path);
        assert(p_u8[-1] == ',');
        assert(p_u8[-2] == '.');
        p_u8[-2] = NULLCH;

        p_main_app = user_path;
        }

    if(!e)
        {
        /* now run app */
        if((e = _kernel_fwrite0(os_file_handle, "Run ")) == NULL)
        if((e = _kernel_fwrite0(os_file_handle, p_main_app)) == NULL)
            e = _kernel_fwrite0(os_file_handle, "\x0A");
        }

    if(e)
        {
        wimpt_complain(e);

        /* ack the message to stop desktop save and remove file */
        wimpt_ack_message(m);
        }

    return(processed);
}

static BOOL
iconbar_PD_DDE(
    wimp_msgstr *m)
{
    BOOL processed = TRUE;
    wimp_t task = m->hdr.task; /* caller's task id */
    S32 id = m->data.pd_dde.id;

    trace_1(TRACE_APP_PD4, "ukprocessor got PD DDE message %d", id);

    switch(id)
        {
        case Wimp_MPD_DDE_IdentifyMarkedBlock:
            {
            trace_0(TRACE_APP_PD4, "IdentifyMarkedBlock");

            if((blkstart.col != NO_COL)  &&  (blkend.col != NO_COL))
                {
                wimp_msgstr msg;
                char *ptr = msg.data.pd_dde.type.a.text;
                size_t nbytes = sizeof(WIMP_MSGPD_DDETYPEA_TEXT)-1;
                ghandle ghan;
                S32 xsize = (S32) (blkend.col - blkstart.col);
                S32 ysize = (S32) (blkend.row - blkstart.row);
                const char *leaf;
                const char *tag;
                S32 taglen;
                P_CELL tcell;

                select_document_using_docno(blk_docno);

                (void) mergebuf_nocheck();
                filbuf();

                leaf = file_leafname(currentfilename);
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
                    ghan = graph_add_entry(m->hdr.my_ref,        /* unique number */
                                           blk_docno, blkstart.col, blkstart.row,
                                           xsize, ysize, leaf, tag,
                                           (int) task);

                    if(ghan > 0)
                        {
                        trace_5(TRACE_APP_PD4, "IMB: ghan %d xsize %d ysize %d leafname %s tag %s]",
                                ghan, xsize, ysize, leaf, tag);

                        msg.data.pd_dde.id                = Wimp_MPD_DDE_ReturnHandleAndBlock;
                        msg.data.pd_dde.type.a.handle    = ghan;
                        msg.data.pd_dde.type.a.xsize    = xsize;
                        msg.data.pd_dde.type.a.ysize    = ysize;

                        /* send message as ack to his one */
                        msg.hdr.size     = ptr - (char *) &msg;
                        msg.hdr.your_ref = m->hdr.my_ref;
                        msg.hdr.action     = Wimp_MPD_DDE;
                        wimpt_safe(wimp_sendmessage(wimp_ESENDWANTACK, &msg, task));
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
            S32 xsize = m->data.pd_dde.type.a.xsize;
            S32 ysize = m->data.pd_dde.type.a.ysize;
            const char *tstr = m->data.pd_dde.type.a.text;
            const char *tag  = tstr + strlen(tstr) + 1;
            DOCNO docno;
            COL col;
            ROW row;
            P_CELL tcell;

            trace_4(TRACE_APP_PD4, "EstablishHandle: xsize %d, ysize %d, name %s, tag %s", xsize, ysize, tstr, tag);

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

            while((tcell = next_slot_in_block(DOWN_COLUMNS)) != NULL)
                {
                if(tcell->type != SL_TEXT)
                    continue;

                if(0 != _stricmp(tcell->content.text, tag))
                    continue;

                col = in_block.col;
                row = in_block.row;

                /* add entry to list */
                ghan = graph_add_entry(m->hdr.my_ref,        /* unique number */
                                       docno, col, row,
                                       xsize, ysize, tstr, tag,
                                       (int) task);

                if(ghan > 0)
                    {
                    trace_5(TRACE_APP_PD4, "EST: ghan %d xsize %d ysize %d name %s tag %s]",
                            ghan, xsize, ysize, tstr, tag);

                    m->data.pd_dde.id            = Wimp_MPD_DDE_ReturnHandleAndBlock;
                    m->data.pd_dde.type.a.handle = ghan;

                    /* send same message as ack to his one */
                    m->hdr.your_ref = m->hdr.my_ref;
                    m->hdr.action   = Wimp_MPD_DDE;
                    wimpt_safe(wimp_sendmessage(wimp_ESENDWANTACK, m, task));
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
            ghandle han = m->data.pd_dde.type.b.handle;
            P_GRAPHICS_LINK_ENTRY glp;

            trace_2(TRACE_APP_PD4, "%sRequestUpdates: handle %d", (id == Wimp_MPD_DDE_StopRequestUpdates) ? "Stop" : "", han);

            glp = graph_search_list(han);

            if(glp)
                {
                /* stop caller getting bounced msg */
                wimpt_ack_message(m);

                /* flag that updates are/are not required on this handle */
                glp->update = (id == Wimp_MPD_DDE_RequestUpdates);
                }
            }
            break;

        case Wimp_MPD_DDE_RequestContents:
            {
            ghandle han = m->data.pd_dde.type.b.handle;
            P_GRAPHICS_LINK_ENTRY glp;

            trace_1(TRACE_APP_PD4, "RequestContents: handle %d", han);

            glp = graph_search_list(han);

            if(glp)
                {
                /* stop caller getting bounced msg */
                wimpt_ack_message(m);

                select_document_using_docno(glp->docno);

                (void) mergebuf_nocheck();
                filbuf();

                /* fire off all the cells */
                riscos_sendallslots(glp);

                riscos_sendendmarker(glp);
                }
            }
            break;

        case Wimp_MPD_DDE_GraphClosed:
            {
            ghandle han = m->data.pd_dde.type.b.handle;
            PC_GRAPHICS_LINK_ENTRY glp;

            trace_1(TRACE_APP_PD4, "GraphClosed: handle %d", han);

            glp = graph_search_list(han);

            if(glp)
                {
                /* stop caller getting bounced msg */
                wimpt_ack_message(m);

                /* delete entry from list */
                graph_remove_entry(han);
                }
            }
            break;

        case Wimp_MPD_DDE_DrawFileChanged:
            {
            const char *drawfilename = m->data.pd_dde.type.d.leafname;

            trace_1(TRACE_APP_PD4, "DrawFileChanged: name %s", drawfilename);

            /* don't ack this message: other people may want to see it too */

            /* look for any instances of this DrawFile; update windows with refs */
            gr_cache_recache(drawfilename);
            }
            break;

        default:
            trace_1(TRACE_APP_PD4, "ignoring PD DDE type %d message", id);
            processed = FALSE;
            break;
        }

    return(processed);
}

static BOOL
iconbar_message(
    wimp_msgstr *m)
{
    BOOL processed = TRUE;

    switch(m->hdr.action)
        {
        case wimp_MPREQUIT:
            processed = iconbar_PREQUIT(m);
            break;

        case wimp_SAVEDESK:
            /* SKS 26oct96 added */
            processed = iconbar_SAVEDESK(m);
            break;

        case wimp_MDATASAVE:
            /* initial drag of data from somewhere to icon */
            scraptransfer_file(&m->data.datasave, 1);
            break;

        case wimp_MDATAOPEN:
            /* double-click on object */
            processed = iconbar_message_DATAOPEN(m);
            break;

        case wimp_MDATALOAD:
            /* object dragged to icon bar - load mostly regardless of type */
            processed = iconbar_message_DATALOAD(m);
            break;

        case wimp_MPrintTypeOdd:
            {
            /* printer broadcast */
            char * filename;
            FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkprint(&filename);

            trace_1(TRACE_APP_PD4, "ukprocessor got asked if it can print a file of type &%4.4X", filetype);

            if(pd_can_print(filetype))
                {
                print_file(filename);

                xferrecv_printfileok(-1);    /* print all done */
                }
            }
            break;

        case wimp_MPrinterChange:
            trace_0(TRACE_APP_PD4, "ukprocessor got informed of printer driver change");
            riscprint_set_printer_data();
            break;

        case wimp_MMODECHANGE:
            cachemodevariables();
            cachepalettevariables();
            break;

        case wimp_PALETTECHANGE:
            cachepalettevariables();
            draw_redraw_all_pictures();
            break;

        case wimp_MINITTASK:
          if(g_kill_duplicates)
          {
              static int seen_my_birth = FALSE;
              const char *taskname = m->data.taskinit.taskname;
              trace_1(TRACE_APP_PD4, "MTASKINIT for %s", taskname);
              if(0 == strcmp(wimpt_get_taskname(), taskname))
              {
                  if(seen_my_birth)
                  {
                      wimp_msgstr msg;
                      msg.hdr.size = sizeof(wimp_msghdr);
                      msg.hdr.your_ref = 0; /* fresh msg */
                      msg.hdr.action = wimp_MCLOSEDOWN;
                      trace_1(TRACE_APP_PD4, "Another %s is trying to start! I'll kill it", taskname);
                      wimpt_safe(wimp_sendmessage(wimp_ESEND, &msg, m->hdr.task));
                  }
                  else
                  {
                      trace_0(TRACE_APP_PD4, "witnessing our own birth");
                      seen_my_birth = TRUE;
                  }
              }
          }
        break;

        case wimp_MHELPREQUEST:
            trace_0(TRACE_APP_PD4, "ukprocessor got help request for icon bar icon");
            m->data.helprequest.m.i = 0;
            riscos_sendhelpreply(m, help_iconbar);
            break;

        case Wimp_MPD_DDE:
            processed = iconbar_PD_DDE(m);
            break;

        default:
            trace_1(TRACE_APP_PD4, "unprocessed %s message to iconbar handler",
                    report_wimp_message(m, FALSE));
            processed = FALSE;
            break;
        }

    return(processed);
}

static BOOL
iconbar_PD_DDE_bounced(
    wimp_msgstr *m)
{
    BOOL processed = TRUE;
    const S32 id = m->data.pd_dde.id;

    trace_1(TRACE_APP_PD4, "ukprocessor got bounced PD DDE message %d", id);

    switch(id)
        {
        case Wimp_MPD_DDE_SendSlotContents:
            {
            ghandle han = m->data.pd_dde.type.c.handle;
            P_GRAPHICS_LINK_ENTRY glp;
            trace_1(TRACE_APP_PD4, "SendSlotContents on handle %d bounced - receiver dead", han);
            glp = graph_search_list(han);
            if(glp)
                /* delete entry from list */
                graph_remove_entry(han);
            }
            break;

        case Wimp_MPD_DDE_ReturnHandleAndBlock:
            {
            ghandle han = m->data.pd_dde.type.a.handle;
            P_GRAPHICS_LINK_ENTRY glp;
            trace_1(TRACE_APP_PD4, "ReturnHandleAndBlock on handle %d bounced - receiver dead", han);
            glp = graph_search_list(han);
            if(glp)
                /* delete entry from list */
                graph_remove_entry(han);
            }
            break;

        default:
            trace_1(TRACE_APP_PD4, "ignoring bounced PD DDE type %d message", id);
            processed = FALSE;
            break;
        }

    return(processed);
}

static BOOL
iconbar_message_bounced(
    wimp_msgstr *m)
{
    BOOL processed = TRUE;

    switch(m->hdr.action)
        {
        case Wimp_MPD_DDE:
            processed = iconbar_PD_DDE_bounced(m);
            break;

        default:
            trace_1(TRACE_APP_PD4, "unprocessed %s bounced message to iconbar handler",
                    report_wimp_message(m, FALSE));
            processed = FALSE;
            break;
        }

    return(processed);
}

/******************************************************************************
*
* process unhandled events
*
******************************************************************************/

static BOOL
default_event_ENULL(void)
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
default_event_handler(
    wimp_eventstr *e,
    void *handle)
{
    BOOL processed = TRUE;

    IGNOREPARM(handle);

    select_document(NO_DOCUMENT);

    switch(e->e)
        {
        case wimp_ENULL:
            processed = default_event_ENULL();
            break;

        case wimp_EBUT:
            /* one presumes only the icon bar stuff gets here ... */
            iconbar_event_EBUT(e);
            break;

        case wimp_EUSERDRAG:
            trace_0(TRACE_APP_PD4, "UserDrag: stopping drag as button released");

            /* send this to the right guy */
            select_document_using_docno(drag_docno);

            application_drag(e->data.dragbox.x0, e->data.dragbox.y0, TRUE);

            ended_drag();        /* will release nulls */
            break;

        case wimp_ESEND:
        case wimp_ESENDWANTACK:
            processed = iconbar_message(&e->data.msg);
            break;

        case wimp_EACK:
            processed = iconbar_message_bounced(&e->data.msg);
            break;

        default:
            trace_1(TRACE_APP_PD4, "unprocessed wimp event %s", report_wimp_event(e->e, &e->data));
            processed = FALSE;
            break;
        }

    return(processed);
}

/******************************************************************************
*
*  process open window request for rear_window
*
******************************************************************************/

static void
rear_open_request(
    wimp_eventstr *e)
{
    trace_0(TRACE_APP_PD4, "rear_open_request()");

    application_open_request(e);
}

/******************************************************************************
*
* process scroll window request for rear_window
*
******************************************************************************/

static void
rear_scroll_request(
    wimp_eventstr *e)
{
    trace_0(TRACE_APP_PD4, "rear_scroll_request()");

    application_scroll_request(e);
}

/******************************************************************************
*
* process close window request for rear_window
*
******************************************************************************/

/* if       select-close then close window */
/* if       adjust-close then open parent directory and close window */
/* if shift-adjust-close then open parent directory */

static void
rear_close_request(
    wimp_eventstr *e)
{
    BOOL adjustclicked = riscos_adjustclicked();
    BOOL shiftpressed  = host_shift_pressed();
    BOOL justopening   = (shiftpressed  &&  adjustclicked);
    BOOL wanttoclose   = !justopening;

    IGNOREPARM(e);

    trace_0(TRACE_APP_PD4, "rear_close_request()");

    if(!justopening)
        {
        /* deal with modified files etc before opening any filer windows */
        wanttoclose = riscdialog_warning();

        (void) mergebuf_nocheck();
        filbuf();

        if(wanttoclose)
            wanttoclose = save_existing();

        if(wanttoclose)
            wanttoclose = dependent_files_warning();

        if(wanttoclose)
            wanttoclose = dependent_charts_warning();

        if(wanttoclose)
            wanttoclose = dependent_links_warning();
        }

    if(adjustclicked)
        if(file_is_rooted(currentfilename))
            filer_opendir(currentfilename);

    if(wanttoclose)
        close_window();
}

/******************************************************************************
*
* process redraw window request for main_window
*
******************************************************************************/

static void
main_redraw_request(
    wimp_eventstr *e)
{
    os_error * bum;
    wimp_redrawstr redraw;
    S32 redrawindex;

    trace_0(TRACE_APP_PD4, "main_redraw_request()");

    redraw.w = e->data.o.w;

    /* wimp errors in redraw are fatal */
    bum = wimpt_complain(wimp_redraw_wind(&redraw, &redrawindex));

#if TRACE_ALLOWED
    if(!redrawindex)
        trace_0(TRACE_APP_PD4, "no rectangles to redraw");
#endif

    while(!bum  &&  redrawindex)
        {
        killcolourcache();

        graphics_window = * ((const GDI_BOX *) &redraw.g);

        #if defined(ALWAYS_PAINT)
        paint_is_update = TRUE;
        #else
        paint_is_update = FALSE;
        #endif

        /* redraw area, not update */
        application_redraw((RISCOS_REDRAWSTR *) &redraw);

        bum = wimpt_complain(wimp_get_rectangle(&redraw, &redrawindex));
        }
}

/******************************************************************************
*
* process key pressed event for main_window
*
******************************************************************************/

#define CTRL_H    ('H' & 0x1F)    /* Backspace key */
#define CTRL_M    ('M' & 0x1F)    /* Return/Enter keys */
#define CTRL_Z    ('Z' & 0x1F)    /* Last Ctrl-x key */

static void
main_key_pressed(
    wimp_eventstr *e)
{
    S32 ch = e->data.key.chcode;
    S32 kh;

    trace_1(TRACE_APP_PD4, "main_key_pressed: key &%3.3X", ch);

    /* Translate key from Window manager into what PipeDream expects */

      if(ch == ESCAPE)
        kh = ch;
    else if(ch <= CTRL_Z)                /* RISC OS 'Hot Key' here we come */
        {
        kh = (ch - 1 + 'A') | ALT_ADDED; /* Uppercase Alt-letter by default */

        /* Watch out for useful CtrlChars not produced by Ctrl-letter */

        /* SKS after 4.11 10jan92 - if already in Alt-sequence then these go in as Alt-letters
         * without considering Ctrl-state (allows ^BM, ^PHB etc from !Chars but not ^M, ^H still)
        */
        if( ((ch == CTRL_H)  ||  (ch == CTRL_M))  &&
            (alt_array[0] == NULLCH)  &&
            !host_ctrl_pressed())
            {
            if(ch == CTRL_H)
                kh = RISCOS_BACKSPACE_KEY;
            else
                kh = ch;
            }
        }
    else
        {
        if((ch & ~0x00FF) == 0x0100)
            {
            /* convert RISC OS Fn keys to our internal representations */
            S32 shift_added = 0;
            S32 ctrl_added  = 0;

            kh = ch ^ 0x180;       /* map F0 (0x180) to 0x00, F10 (0x1CA) to 0x4A etc. */

            /* remap RISC OS shift and control bits to our definitions */
            if(kh & 0x10)
                {
                kh ^= 0x10;
                shift_added = SHIFT_ADDED;
                }
            else
                shift_added = 0;

            if(kh & 0x20)
                {
                kh ^= 0x20;
                ctrl_added = CTRL_ADDED;
                }
            else
                ctrl_added = 0;

            /* map F10-F15 range onto end of F0-F9 range and map TAB-up arrow out of that range */
            if(     (kh >= 0x4A) && (kh <= 0x4F))
                kh ^= (0x00 ^ 0x40);
            else if((kh >= 0x0A) && (kh <= 0x0F))
                kh ^= (0x10 ^ 0x00);

            kh |= (FN_ADDED | shift_added | ctrl_added);
            }
        else if((alt_array[0] != NULLCH)  &&  isalpha(ch))
                                            /* already in Alt-sequence? */
            kh = toupper(ch) | ALT_ADDED;   /* Uppercase Alt-letter */
        else
            kh = ch;
        }

    /* transform the simple Delete and Home keys into function-like keys */
    if((kh == RISCOS_DELETE_KEY) || (kh == RISCOS_HOME_KEY) || (kh == RISCOS_BACKSPACE_KEY))
        {
        S32 shift_added = host_shift_pressed() ? SHIFT_ADDED : 0;
        S32 ctrl_added  = host_ctrl_pressed()  ? CTRL_ADDED  : 0;

        if(     kh == RISCOS_DELETE_KEY)
            kh = DELETE_KEY;
        else if(kh == RISCOS_HOME_KEY)
            kh = HOME_KEY;
        else if(kh == RISCOS_BACKSPACE_KEY)
            kh = BACKSPACE_KEY;
        else
            assert(0);

        kh |= (shift_added | ctrl_added);
        }

    if(!application_process_key(kh))
        {
        /* if unprocessed, send it back from whence it came */
        trace_1(TRACE_APP_PD4, "main_key_pressed: unprocessed app_process_key(&%8X)", ch);
        wimpt_safe(wimp_processkey(ch));
        }
}

/******************************************************************************
*
*  process message events for main_window
*
******************************************************************************/

static BOOL
draw_insert_filename(
    PC_U8 filename)
{
    char cwd_buffer[BUF_MAX_PATHSTRING];

    trace_0(TRACE_APP_PD4, "inserting minimalist reference to graphic/chart file as text-at G field");

    /* try for minimal reference */
    if(NULL != file_get_cwd(cwd_buffer, elemof32(cwd_buffer), currentfilename))
        {
        size_t cwd_len = strlen(cwd_buffer);

        if(0 == _strnicmp(filename, cwd_buffer, cwd_len))
            filename += cwd_len;
        }

    filbuf();

    if(NULLCH != get_text_at_char())
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

static BOOL
main_DATALOAD(
    wimp_msgstr *m)
{
    BOOL processed = TRUE;
    char * filename;
    FILETYPE_RISC_OS filetype = (FILETYPE_RISC_OS) xferrecv_checkinsert(&filename); /* sets up reply too */
    STATUS filetype_option;

    IGNOREPARM(m); /* xferrecv uses last message mechanism */

    reportf("MDATALOAD(main): file type &%4.4X, name %u:%s", filetype, strlen32(filename), filename);

    switch(filetype)
        {
        case FILETYPE_DIRECTORY:
        case FILETYPE_APPLICATION:
            reperr(create_error(FILE_ERR_ISADIR), filename);
            break;

        case FILETYPE_PDMACRO:
            if(mystr_set(&d_macro_file[0].textfield, filename))
                {
                do_execfile(d_macro_file[0].textfield);
                str_clr(   &d_macro_file[0].textfield);
                }
            break;

        case FILETYPE_PIPEDREAM:
            filetype_option = find_filetype_option(filename); /* checks readability and discriminates PipeDream chart files */

            if(PD4_CHART_CHAR == filetype_option)
                {
                trace_0(TRACE_APP_PD4, "pd chart about to be loaded via text-at field G mechanism");
                draw_insert_filename(filename);
                break;
                }

            (void) riscos_LoadFile(filename, TRUE, filetype_option /*may be 0,Err*/);

            break;

        default:
            if(gr_cache_can_import(filetype))
                {
                draw_insert_filename(filename);
                break;
                }

            if(filetype == FILETYPE_CSV)
                {
                if((filetype_option = file_readable(filename)) > 0)
                    filetype_option = CSV_CHAR;
                }
            else
                filetype_option = find_filetype_option(filename); /* check the readability & do the auto-detect here for consistency */

            (void) riscos_LoadFile(filename, TRUE, filetype_option /*may be 0,Err*/);

            break;
        }

    /* this is mandatory */
    xferrecv_insertfileok();

    return(processed);
}

static BOOL
main_HELPREQUEST(
    wimp_msgstr *m)
{
    BOOL processed = TRUE;
    int x = m->data.helprequest.m.x;
    int y = m->data.helprequest.m.y;
    P_DOCU p_docu = find_document_with_input_focus();
    BOOL insertref = ( (NO_DOCUMENT != p_docu) &&
                       (p_docu->Xxf_inexpression || p_docu->Xxf_inexpression_box || p_docu->Xxf_inexpression_line) );
    char abuffer[256+128/*bit of slop*/];
    U32 prefix_len;
    char * buffer;
    char * alt_msg;
    coord tx    = tcoord_x(x);    /* text cell coordinates */
    coord ty    = tcoord_y(y);
    coord coff  = calcoff(tx);    /* not _click */
    coord roff  = calroff(ty);    /* not _click */
    coord o_roff = roff;
    ROW trow;
    P_SCRROW rptr;
    BOOL append_drag_msg = xf_inexpression; /* for THIS window */

    if(dragtype != NO_DRAG_ACTIVE) /* stop pointer and message changing whilst dragging */
        return(processed);

    trace_4(TRACE_APP_PD4, "get_slr_for_point: g(%d, %d) t(%d, %d)", x, y, tx, ty);

    /* stop us wandering off bottom of sheet */
    if(roff >= rowsonscreen)
        roff = rowsonscreen - 1;

    trace_1(TRACE_APP_PD4, " roff %d", roff);

    /* default message */
    xstrkpy(abuffer, elemof32(abuffer), help_main_window);

    prefix_len = strlen(abuffer); /* remember a possible cut place */

    alt_msg = buffer = abuffer + prefix_len;

    if(roff >= 0)
        {
        rptr = vertvec_entry(roff);

        if(rptr->flags & PAGE)
            xstrkpy(buffer, elemof32(abuffer) - prefix_len, help_row_is_page_break);
        else
            {
            trow = rptr->rowno;

            if((coff >= 0)  ||  (coff == OFF_RIGHT))
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

                    tcol = col_number(coff);
                    trace_2(TRACE_APP_PD4, "in sheet at row #%d, col #%d", trow, tcol);
                    (void) write_ref(abuf, elemof32(abuf), current_docno(), tcol, trow);

                    if(!insertref)
                        {
                        coff = get_column(tx, trow, 0, TRUE);
                        scol = col_number(coff);
                        trace_2(TRACE_APP_PD4, "will position at row #%d, col #%d", trow, scol);
                        (void) write_ref(sbuf, elemof32(sbuf), current_docno(), scol, trow);
                        }
                    else
                        scol = tcol;

                    (void) xsnprintf(buffer, elemof32(abuffer) - prefix_len,
                            (scol != tcol)
                                ? "%s%s%s" "%s%s"     "%s%s%s"       "%s.|M"
                                : "%s%s%s" "%.0s%.0s" "%.0s%.0s%.0s" "%s.|M",
                            help_click_select_to, msg, help_slot,
                            /* else miss this set of args */
                            sbuf, ".|M",
                            /* and this set of args */
                            help_click_adjust_to, msg, help_slot,
                            abuf);
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
        trace_0(TRACE_APP_PD4, "above sheet data");

    if(append_drag_msg && (strlen32p1(abuffer) + strlen32(help_drag_file_to_insert) < 240))
         xstrkat(abuffer, elemof32(abuffer), help_drag_file_to_insert);

    riscos_sendhelpreply(m, (strlen32p1(abuffer) < 240) ? abuffer : alt_msg);

    return(processed);
}

static BOOL
main_message(
    wimp_msgstr *m)
{
    int action     = m->hdr.action;
    BOOL processed = TRUE;

    switch(action)
        {
        case wimp_MDATASAVE:
            /* possible object dragged into main_window - ask for DATALOAD */
            scraptransfer_file(&m->data.datasave, 0 /* not iconbar */);
            break;

        case wimp_MDATALOAD:
            /* object (possibly scrap) dragged into main_window - insert mostly regardless of type */
            processed = main_DATALOAD(m);
            break;

        case wimp_MHELPREQUEST:
            trace_0(TRACE_APP_PD4, "Help request on main_window");
            processed = main_HELPREQUEST(m);
            break;

        case wimp_MWINDOWINFO:
            /* help out the iconizer - send him a new message as acknowledgement of his request */
            m->hdr.size     = sizeof(m->hdr) + sizeof(m->data.windowinfo);
            m->hdr.your_ref = m->hdr.my_ref;
            m->hdr.action   = wimp_MWINDOWINFO;

            m->data.windowinfo.reserved_0 = 0;
            (void) strcpy(m->data.windowinfo.sprite, /*"ic_"*/ "dde"); /* dde == PipeDream */
            (void) strcpy(m->data.windowinfo.title, file_leafname(currentfilename));

            wimpt_safe(wimp_sendmessage(wimp_ESEND, m, m->hdr.task));
            break;

        default:
            trace_1(TRACE_APP_PD4, "unprocessed %s message to main_window",
                     report_wimp_message(m, FALSE));
            processed = FALSE;
            break;
        }

    return(processed);
}

/******************************************************************************
*
* process main_window events
*
******************************************************************************/

static BOOL
main_event_handler(
    wimp_eventstr *e,
    void *handle)
{
    BOOL processed = TRUE;

    if(!select_document_using_callback_handle(handle))
        {
        messagef(TEXT("Bad handle ") PTR_XTFMT TEXT(" passed to main event handler"), report_ptr_cast(handle));
        return(FALSE);
        }

    trace_4(TRACE_APP_PD4, TEXT("main_event_handler: event %s, dhan ") PTR_XTFMT TEXT(" window %d, document %d"),
             report_wimp_event(e->e, &e->data), report_ptr_cast(handle), (int) main_window, current_docno());

    switch(e->e)
        {
        case wimp_EOPEN:      /* main_window always opened as a pane on top of rear_window */
        case wimp_ECLOSE:     /* main_window has no close box */
        case wimp_ESCROLL:    /* or scroll bars - come through fake */
        case wimp_EPTRLEAVE:
        case wimp_EPTRENTER:
            break;

        case wimp_EREDRAW:
            main_redraw_request(e);
            break;

        case wimp_EBUT:
            /* ignore old button state */
            application_button_click_in_main(e->data.but.m.x,
                                             e->data.but.m.y,
                                             e->data.but.m.bbits
                                            );
            break;

        case wimp_EKEY:
            main_key_pressed(e);
            break;

        case wimp_ESEND:
        case wimp_ESENDWANTACK:
            main_message(&e->data.msg);
            break;

        case wimp_EGAINCARET:
            trace_2(TRACE_APP_PD4, "GainCaret: new window %d icon %d",
                    e->data.c.w, e->data.c.i);
            trace_4(TRACE_APP_PD4, " x %d y %d height %8.8X index %d",
                    e->data.c.x, e->data.c.y,
                    e->data.c.height, e->data.c.index);
            caret_window = e->data.c.w;

            /* This document is gaining the caret, and will show the cell count (recalculation status) from now on */
            if(slotcount_docno != current_docno())
                {
                colh_draw_slot_count_in_document(NULL); /* kill the current indicator (if any) */

                slotcount_docno = current_docno();
                }
            break;

        case wimp_ELOSECARET:
            trace_2(TRACE_APP_PD4, "LoseCaret: old window %d icon %d",
                    e->data.c.w, e->data.c.i);
            trace_4(TRACE_APP_PD4, " x %d y %d height %X index %d",
                    e->data.c.x, e->data.c.y,
                    e->data.c.height, e->data.c.index);

            /* cancel Alt sequence */
            alt_array[0] = NULLCH;

            /* don't cancel key expansion or else user won't be able to
             * set 'Next window', 'other action' etc. on keys
            */
            (void) mergebuf_nocheck();
            filbuf();

            caret_window = window_NULL;

#if FALSE
            colh_draw_slot_count(NULL);

            /*RCM says: I suppose we could find out which doc (if any) is getting the focus */
            /*          and only blank the cell count if its not this doc - this would mean */
            /*          the count wouldn't blank out momentarily if the focus went to an    */
            /*          edit expression box owned by this document                          */
            /*          NB The expression editor would need similar ELOSECARET code to this */
#endif
            break;

        default:
            trace_1(TRACE_APP_PD4, "unprocessed wimp event to main_window: %s",
                    report_wimp_event(e->e, &e->data));
            processed = FALSE;
            break;
        }

    return(processed);
}

static BOOL
rear_event_handler(
    wimp_eventstr *e,
    void *handle)
{
    BOOL processed = TRUE;

    if(!select_document_using_callback_handle(handle))
        {
        messagef(TEXT("Bad handle ") PTR_XTFMT TEXT(" passed to rear event handler"), report_ptr_cast(handle));
        return(FALSE);
        }

    trace_4(TRACE_APP_PD4, TEXT("rear_event_handler: event %s, handle ") PTR_XTFMT TEXT(" window %d, document %d"),
             report_wimp_event(e->e, &e->data), report_ptr_cast(handle), (int) rear_window, current_docno());

    switch(e->e)
        {
        case wimp_EPTRLEAVE:
        case wimp_EPTRENTER:
            break;

        case wimp_EOPEN:
            rear_open_request(e);
            break;

        case wimp_ECLOSE:
            rear_close_request(e);
            break;

        case wimp_ESCROLL:
            rear_scroll_request(e);
            break;

        /* SKS 26oct96 */
        case wimp_ESEND:
        case wimp_ESENDWANTACK:
            main_message(&e->data.msg);
            break;

    /*    case wimp_EREDRAW:    */
        default:
            trace_1(TRACE_APP_PD4, "unprocessed wimp event to rear_window: %s",
                    report_wimp_event(e->e, &e->data));
            processed = FALSE;
            break;
        }

    return(processed);
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

    a[-1] = NULLCH;

    return(str);
}

/******************************************************************************
*
* if window not already in existence
* in this domain then make a new window
*
******************************************************************************/

extern BOOL
riscos_createmainwindow(void)
{
    DOCNO docno = current_docno();
    os_error * e;

    trace_0(TRACE_APP_PD4, "riscos_createmainwindow()");

    if(rear_window == window_NULL) /* a window needs creating? */
        {
        rear_template = template_copy_new(template_find_new(rear_window_name));

        if(NULL == rear_template)
            return(reperr_null(status_nomem()));

        riscos_settitlebar(currentfilename); /* set the buffer content up */

        ((wimp_wind *) (current_p_docu->Xrear_template))->title.indirecttext.buffer  = current_p_docu->Xwindow_title;
        ((wimp_wind *) (current_p_docu->Xrear_template))->title.indirecttext.bufflen = BUF_WINDOW_TITLE_LEN; /* includes terminator */

        if((e = win_create_wind(rear_template, (wimp_w *) &rear_window,
                                rear_event_handler, (void *)(uintptr_t)docno)) != NULL)
            return(rep_fserr(e->errmess));

        /* Bump the y coordinate for the next window to be created */
        /* Try not to overlap the icon bar */
        riscos_setnextwindowpos(readval_S32(&main_window_definition->box.y1));

        /* get extent and scroll offsets set correctly on initial open */
        out_alteredstate = TRUE;
        }

    if(colh_window == window_NULL) /* a window needs creating? */
        {
        colh_template = template_copy_new(template_find_new(colh_window_name));

        if(NULL == colh_template)
            return(reperr_null(status_nomem()));

        if((e = win_create_wind(colh_template, (wimp_w *) &colh_window,
                                colh_event_handler, (void *)(uintptr_t)docno)) != NULL)
            return(rep_fserr(e->errmess));

        colh_position_icons();      /*>>>errors???*/
        }

    if(main_window == window_NULL) /* a window needs creating? */
        {
        main_template = template_copy_new(template_find_new(main_window_name));

        if(NULL == main_template)
            return(reperr_null(status_nomem()));

        if((e = win_create_wind(main_template, (wimp_w *) &main_window,
                                main_event_handler, (void *)(uintptr_t)docno)) != NULL)
            return(rep_fserr(e->errmess));

        riscmenu_attachmenutree(main_window);
        riscmenu_attachmenutree(colh_window);

        /* Create function paster menu */
        function__event_menu_maker();
        event_attachmenumaker_to_w_i(colh_window,
                                    (wimp_i)COLH_FUNCTION_SELECTOR,
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
* destroy this domain's mainwindow
*
******************************************************************************/

extern void
riscos_destroymainwindow(void)
{
    trace_0(TRACE_APP_PD4, "riscos_destroymainwindow()");

    if(rear_window != window_NULL)
        {
        /* deregister procedures for the rear_window */

        win_delete_wind((wimp_w *) &rear_window);
        rear_window = window_NULL;
        wlalloc_dispose(P_P_ANY_PEDANTIC(&rear_template));
        }

    if(colh_window != window_NULL)
        {
        /* deregister procedures for the colh_window */
        riscmenu_detachmenutree(colh_window);

        win_delete_wind((wimp_w *) &colh_window);
        colh_window = window_NULL;
        wlalloc_dispose(P_P_ANY_PEDANTIC(&colh_template));

        /*>>>should probably give caret away, if this window has it */
        }

    if(main_window != window_NULL)
        {
        /* pretty permanent caret loss */
        if(main_window == caret_window)
            caret_window = window_NULL;

        /* deregister procedures for the main_window */
        riscmenu_detachmenutree(main_window);

        win_delete_wind((wimp_w *) &main_window);
        main_window = window_NULL;
        wlalloc_dispose(P_P_ANY_PEDANTIC(&main_template));
        }
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

    riscos_destroymainwindow();
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
* bring main_window to the front
*
******************************************************************************/

extern void
riscos_frontmainwindow(
    BOOL immediate)
{
    trace_1(TRACE_APP_PD4, "riscos_frontmainwindow(immediate = %s)", trace_boolstring(immediate));

    if(main_window != window_NULL)
        {
        xf_frontmainwindow = FALSE;                    /* as immediate event raising calls draw_screen() */
        win_send_front(rear__window, immediate);
        }
}

extern void
riscos_frontmainwindow_atbox(
    BOOL immediate)
{
    trace_1(TRACE_APP_PD4, "riscos_frontmainwindow_atbox(immediate = %s)", trace_boolstring(immediate));

    if(main_window != window_NULL)
        {
        xf_frontmainwindow = FALSE;                    /* as immediate event raising calls draw_screen() */
        win_send_front_at(rear__window, immediate, (const wimp_box *) &open_box);
        }
}

extern S32
riscos_getbuttonstate(void)
{
    wimp_mousestr m;
    wimpt_safe(wimp_get_point_info(&m));
    return(m.bbits);
}

extern BOOL
riscos_adjustclicked(void)
{
    return(riscos_getbuttonstate() & wimp_BRIGHT);
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

    ok = riscos_createmainwindow();

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

extern void
riscos_initialise_once(void)
{
    trace_0(TRACE_APP_PD4, "riscos_initialise_once()");

    resspr_init();

    /* register callproc for wimp exit */
    wimpt_atexit(actions_before_exiting);

    /* no point having more than one of this app loaded */
    g_kill_duplicates = 1;

    /* SKS 25oct96 allow templates to go out */
    template_require(0, "tem_risc");
    template_require(1, "tem_main");
    template_require(2, "tem_cht");
    template_require(3, "tem_bc");
    template_require(4, "tem_el");
    template_require(5, "tem_f");
    template_require(6, "tem_p");
    template_require(7, "tem_s");
    template_init(); /* NB before dbox_init() */
    dbox_init();

    /* now get a handle onto the definition after the dbox is loaded */

    /* now keep main_window_definition pointing to the back window for all time */

    main_window_definition = template_syshandle_ua(rear_window_name);

    main_window_initial_y1     = (int) readval_S32(&main_window_definition->box.y1);
    main_window_default_height = main_window_initial_y1 -
                                 (int) readval_S32(&main_window_definition->box.y0);

    {
    wimp_wind * fontselector_definition = template_syshandle_ua(fontselector_dboxname);
    wimp_icon * fontselector_firsticon  = (wimp_icon *) (fontselector_definition + 1);

    writeval_S32(&fontselector_firsticon[FONT_LEADING].flags,
     readval_S32(&fontselector_firsticon[FONT_LEADING].flags)      | wimp_INOSELECT);
    writeval_S32(&fontselector_firsticon[FONT_LEADING_DOWN].flags,
     readval_S32(&fontselector_firsticon[FONT_LEADING_DOWN].flags) | wimp_INOSELECT);
    writeval_S32(&fontselector_firsticon[FONT_LEADING_UP].flags,
     readval_S32(&fontselector_firsticon[FONT_LEADING_UP].flags)   | wimp_INOSELECT);
    writeval_S32(&fontselector_firsticon[FONT_LEADING_AUTO].flags,
     readval_S32(&fontselector_firsticon[FONT_LEADING_AUTO].flags) | wimp_INOSELECT);
    }

    iconbar_initialise(product_id());

    /* Direct miscellaneous unknown (and icon bar)
     * events to our default event handler
    */
    win_register_new_event_handler(win_ICONBARLOAD,
                                   default_event_handler,
                                   NULL);
    win_claim_unknown_events(win_ICONBARLOAD);

    /* initialise riscmenu */
    riscmenu_initialise_once();

    /* initialise riscdraw */
    cachemodevariables();
    cachepalettevariables();

    riscprint_set_printer_data();

    riscos__initialised = TRUE;
}

/******************************************************************************
*
* invalidate all of main_window & colh_window
*
******************************************************************************/

extern void
riscos_invalidatemainwindow(void)
{
    wimp_redrawstr redraw;

    if(main__window != window_NULL)
        {
        redraw.w      = main__window;
        redraw.box.x0 = -0x07FFFFFF;
        redraw.box.y0 = -0x07FFFFFF;
        redraw.box.x1 = +0x07FFFFFF;
        redraw.box.y1 = +0x07FFFFFF;

        wimpt_safe(wimp_force_redraw(&redraw));
        }

    if(colh_window != window_NULL)
        {
        colh_colour_icons();                    /* ensure colours of wimp maintained icons are correct */

        redraw.w      = colh_window;
        redraw.box.x0 = -0x07FFFFFF;
        redraw.box.y0 = -0x07FFFFFF;
        redraw.box.x1 = +0x07FFFFFF;
        redraw.box.y1 = +0x07FFFFFF;

        wimpt_safe(wimp_force_redraw(&redraw)); /* will force icons maintained by us to be redrawn */
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
        wimpt_complain((os_error *) _kernel_last_oserror());

    if(res != OSFile_ObjectType_File)
        {
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
* restore the current window to the saved position
*
******************************************************************************/

extern void
riscos_restorecurrentwindowpos(
    BOOL immediate)
{
    wimp_wstate wstate;

    wstate.o = saved_window_pos;

    /* send myself an open request for this new window */
    win_send_open(rear__window, immediate, &wstate.o);
}

extern void
riscos_zerocurrentwindowscrolls(void)
{
    saved_window_pos.scx = 0;
    saved_window_pos.scy = 0;
}

/******************************************************************************
*
* save the current window's position
*
******************************************************************************/

extern void
riscos_savecurrentwindowpos(void)
{
    wimp_eventdata ed;

    /* try to preserve the window coordinates */
    wimpt_safe(wimp_get_wind_state((wimp_w) rear_window,
                                   (wimp_wstate *) &ed.o));

    saved_window_pos = ed.o;
}

/******************************************************************************
*
* send a help reply
*
******************************************************************************/

extern void
riscos_sendhelpreply(
    wimp_msgstr * m,
    const char * msg)
{
    S32 size, length;

    if((int) m->data.helprequest.m.i >= -1)
        {
        length = strlen(msg) + 1;
        size = offsetof(wimp_msgstr, data.helpreply.text) + length;

        m->hdr.size     = size;
        m->hdr.your_ref = m->hdr.my_ref;
        m->hdr.action   = wimp_MHELPREPLY;

        trace_2(TRACE_APP_PD4, "helpreply is %d long, %s", size, msg);

        xstrkpy(m->data.helpreply.text, 256 - offsetof(wimp_msgstr, data.helpreply.text), msg);

        wimpt_safe(wimp_sendmessage(wimp_ESEND, m, m->hdr.task));
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

    if((this_y1 - main_window_default_height) <= (iconbar_height + wimpt_hscroll_height()))
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

    trace_2(TRACE_APP_PD4, "riscos_settitlebar(%s): rear_window %d", documentname, rear_window);

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

    if(rear_window != window_NULL)
        win_changedtitle(rear__window);
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
* invalidate part of main_window and
* call application back to redraw it
* NB. this takes offsets from xorg/yorg
*
******************************************************************************/

extern void
riscos_updatearea(
    RISCOS_REDRAWPROC redrawproc,
    wimp_w w,
    int x0,
    int y0,
    int x1,
    int y1)
{
    os_error * bum;
    wimp_redrawstr redraw;
    S32 redrawindex;

    trace_6(TRACE_APP_PD4, "riscos_updatearea(%s, %d, %d, %d, %d, %d)",
             report_procedure_name(report_proc_cast(redrawproc)), w, x0, y0, x1, y1);

    /* RISC OS graphics primitives all need coordinates limited to s16 */
    x0 = MAX(x0, SHRT_MIN);
    y0 = MAX(y0, SHRT_MIN);
    x1 = MIN(x1, SHRT_MAX);
    y1 = MIN(y1, SHRT_MAX);

    redraw.w      = w;
    redraw.box.x0 = x0;
    redraw.box.y0 = y0;
    redraw.box.x1 = x1;
    redraw.box.y1 = y1;

    /* wimp errors in update are fatal */
    bum = wimpt_complain(wimp_update_wind(&redraw, &redrawindex));

#if TRACE_ALLOWED
    if(!redrawindex)
        trace_0(TRACE_APP_PD4, "no rectangles to update");
#endif

    while(!bum  &&  redrawindex)
        {
        killcolourcache();

        graphics_window = * ((const GDI_BOX *) &redraw.g);

        #if !defined(ALWAYS_PAINT)
        paint_is_update = TRUE;
        #endif

        /* we are updating area */
        redrawproc((RISCOS_REDRAWSTR *) &redraw);

        bum = wimpt_complain(wimp_get_rectangle(&redraw, &redrawindex));
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
        wimpt_complain((os_error *) _kernel_last_oserror());
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

/* end of riscos.c */
