/* pdmain.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Module that drives PipeDream */

/* RJM Aug 1987 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#ifndef __cs_event_h
#include "cs-event.h"   /* includes event.h */
#endif

#ifndef __res_h
#include "res.h"        /* includes <stdio.h> */
#endif

#include <locale.h>     /* for setlocale */
#include <signal.h>     /* for SIGSTAK */

#include "riscos_x.h"
#include "ridialog.h"
#include "pd_x.h"

#include "cmodules/wm_event.h"

#ifndef __swis_h
#include "swis.h" /* C: */
#endif

#include "cs-flex.h"

/* Install a new ESCAPE handler, replacing that set by the C runtime library.
 *
 * When an ESCAPE condition is detected, the flag word is set non-zero.
 * When the ESCAPE condition is cleared, the flag word is set to zero.
 *
 * To clear an ESCAPE condition, use OS_Byte 124.
 * To acknowledge (and clear) an ESCAPE condition, use OS_Byte 126.
 * To generate an ESCAPE condition, use OS_Byte 125.
*/
extern void
EscH(
    int *addressofflag);

/* Install a new event handler, replacing that set by the C runtime library.
*/
extern void *
EventH(void);

/*
internal functions
*/

static void
application__atexit(void);

#if defined(APCS_SOFTPCS)
static void
application__atexit_apcs_softpcs(void);
#endif

static void
decode_command_line_options(
    _InVal_     int argc,
    char * argv[],
    _InVal_     int pass);

static void
decode_run_options(void);

static void
exec_pd_key_once(void);

static void
host_initialise_file_path(void);

static U8 product_name[16] = "PipeDream";
static U8 product_ui_name[16] = "PipeDream";
static U8 product_spritename[16] = "!pipedream";

static U8 __user_name[64] = "Colton Software";
static U8 __organisation_name[64];
static U8 __registration_number[4*4 + 1]; /* NB no spaces */

const PC_U8Z
g_dynamic_area_name = "PipeDream workspace";

#ifdef PROFILING

#include "cmodules/riscos/profile.h"

/* called by atexit() on exit(rc) */

static void
profile_atexit(void)
{
    profile_off();

    /*profile_output(PROFILE_SORT_BY_ADDR, NULL, NULL);*/ /* send just to Reporter */
    /*profile_output(PROFILE_SORT_BY_ADDR,  "$.Temp.pro_addr", NULL);*/
    /*profile_output(PROFILE_SORT_BY_COUNT, "$.Temp.pro_coun", ",");*/
    /*profile_output(PROFILE_SORT_BY_NAME,  "$.Temp.pro_name", "\t");*/

    /*profile_fini(); let the !RunProf script handle the required dump and unload*/

    /*_fmapstore("$.Temp.pro_map");*/
}

static void
profile_setup(void)
{
    profile_init();

    /*profile_on();*/

    atexit(profile_atexit);
}

#endif /* PROFILING */

/*
Read details from Info file - variables are now set up by Loader
*/

static void
get_user_info(void)
{
    TCHARZ var_name[BUF_MAX_PATHSTRING];
    TCHARZ env_value[256];

    if(NULL == _kernel_getenv(make_var_name(var_name, elemof32(var_name), "$User1"), env_value, elemof32(env_value)))
        if(CH_NULL != env_value[0])
            xstrkpy(__user_name, elemof32(__user_name), env_value);

    if(NULL == _kernel_getenv(make_var_name(var_name, elemof32(var_name), "$User2"), env_value, elemof32(env_value)))
        xstrkpy(__organisation_name, elemof32(__organisation_name), env_value);

    if(NULL == _kernel_getenv(make_var_name(var_name, elemof32(var_name), "$RegNo"), env_value, elemof32(env_value)))
        xstrkpy(__registration_number, elemof32(__registration_number), env_value);
}

#if defined(UNUSED)

static int
pd_stack_overflow_test(int i)
{
    unsigned char buf[256];
    unsigned int j;
    for(j = 0; j < 256; ++j)
        buf[j] = i & 0xFF;
    return(buf[i & 0xFF] + pd_stack_overflow_test(i + 1));
}

#endif

/******************************************************************************
*
* Application starts here
*
******************************************************************************/

/* The list of messages that this application is interested in */

static /*const*/ int
pd_wimp_messages[] =
{
    Wimp_MDataSave,
    Wimp_MDataSaveAck,
    Wimp_MDataLoad,
    Wimp_MDataLoadAck,
    Wimp_MDataOpen,
    Wimp_MPreQuit,
    Wimp_MPaletteChange,
    Wimp_MSaveDesktop,
    Wimp_MShutDown,

    Wimp_MHelpRequest,

    Wimp_MPD_DDE,

    Wimp_MMenuWarning,
    Wimp_MModeChange,
    Wimp_MTaskInitialise,
    Wimp_MMenusDeleted,
    Wimp_MWindowInfo,

    Wimp_MPrintFile,
    Wimp_MPrintSave,
    Wimp_MPrintError,
    Wimp_MPrintTypeOdd,

    Wimp_MQuit /* terminate list with zero */
};

static jmp_buf event_loop_jmp_buf;

static void
pd_report_and_trace_enable(void)
{
    /* allow application to run without any report info */
    char env_value[BUF_MAX_PATHSTRING];

    report_enable(NULL == _kernel_getenv("PipeDream$ReportEnable", env_value, elemof32(env_value)));

#if TRACE_ALLOWED
    /* allow trace version to run without any trace info */
    if(NULL != _kernel_getenv("PipeDream$TraceEnable", env_value, elemof32(env_value)))
    {
        trace_disable();
    }
    else
    {
        _kernel_stack_chunk *scptr;
        unsigned long scsize;
        int val = atoi(env_value);
        if(0 != val)
        {
            trace_on();
            trace_1(TRACE_APP_PD4, "main: sp ~= " PTR_XTFMT, report_ptr_cast(&env_value[0]));
            scptr = _kernel_current_stack_chunk();
            scsize = scptr->sc_size;
            trace_3(TRACE_APP_PD4, "main: stack chunk " PTR_XTFMT ", size %lu, top " PTR_XTFMT, report_ptr_cast(scptr), scsize, report_ptr_cast((char *) scptr + scsize));
        }
    }
#endif /* TRACE_ALLOWED */
}

extern int
main(
    int argc,
    char * argv[])
{
    #if defined(NO_SURRENDER)
    /* ensure ESCAPE enabled for trapping */
    (void) _kernel_osbyte(229, 0, 0);
    #else
    /* Trap ESCAPE as soon as possible */
    EscH(&ctrlflag);
    #endif

    /* block events from reaching C runtime system where they cause enormous chugging */
    EventH();

#ifdef PROFILING
    profile_setup();
#endif

    muldiv64_init(); /* very early indeed */

    wimptx_os_version_determine(); /* very early indeed */

    pd_report_and_trace_enable();

    get_user_info();

    decode_command_line_options(argc, argv, 1);

    decode_run_options();

    // for 4.62 ev_set_C99_mode(TRUE);

    /* startup Window Manager interface */
    wimptx_set_spritename(product_spritename); /* For RISC OS 3.5 and later error reporting */
    wimptx_set_taskname(product_ui_id()); /* Optional: uses wimpt_init() parm when omitted */

    riscos_hourglass_on();

    /* keep world valid during initialisation */
    docu_array_init_once();

    /* need to have set up the resource place and the program name for all to work ok */
    res_init("PipeDream"); /* RISC_OSLib-type resources may be found along PipeDream$Path */

    /* Startup Window Manager interface */
    wimpt_wimpversion(310);
    wimpt_messages((wimp_msgaction *) pd_wimp_messages);
    wimpt_init(de_const_cast(char *, product_ui_id()));

#ifdef PROFILING
    /* Task ID may now have changed, so re-register */
    profile_taskchange();
#endif

#if defined(APCS_SOFTPCS)
    apcs_softpcs_initialise(); /* not too early */

    if(atexit(application__atexit_apcs_softpcs)) /* ensure closedown proc called on exit */
        return(EXIT_FAILURE);
#endif

    if(status_fail(aligator_init()))
    {
        /* use base allocator to get error out */
        strings_init();
        reperr_init();

        consume_bool(reperr_null(ERR_NOTINDESKTOP));
        return(EXIT_FAILURE);
    }

    if(atexit(application__atexit)) /* ensure closedown proc called on exit */
        return(EXIT_FAILURE);

    strings_init();
    reperr_init();

    host_initialise_file_path();

    file_init();

    /* once off initialisations */

    riscos_initialise_once();

    riscdialog_initialise_once();   /* may werr() if really duff */

    init_mc();                      /* old style error reporting now possible */

    /* which parts of PipeDream are present */
    headline_initialise_once();

    /* initialise DOCU template */
    docu_init_once();

    /* loads ini file if we have one */
    constr_initialise_once();

    /* exec key file if we have one */
    exec_pd_key_once();

    /* decode command line arguments */
    decode_command_line_options(argc, argv, 2);

    /* put the caret in a document if there is one (left over from key file etc.) */
    if(NO_DOCUMENT != first_document())
    {
        select_document(first_document());
        /* get caret when it is actually fronted */
        xf_acquirecaret = TRUE;
        xf_front_document_window = TRUE;
        draw_screen();
    }

    /* turn off null events unless we really want them - tested at the exit of each event processed */
    event_setmask((wimp_emask) (event_getmask() | Wimp_Poll_NullMask));

    /* set up a point we can longjmp back to on serious error */
    switch(setjmp(event_loop_jmp_buf))
    {
    case SIGSTAK:
        wimptx_stack_overflow_handler(SIGSTAK);

        /*FALLTHRU*/

    default: /* returned here from exception - tidy up as necessary */

        /* SKS after PD 4.11 31jan92 */
        riscos_printing = FALSE;
        cmd_seq_cancel(); /* cancel Cmd-sequence */

        trace_0(TRACE_APP_PD4, TEXT("main: Starting to poll for messages again after serious error"));
        break;

    case 0:
        wimptx_set_safepoint(&event_loop_jmp_buf);

        trace_0(TRACE_APP_PD4, TEXT("main: Starting to poll for messages"));

        /*pd_stack_overflow_test(0);*/
        break;
    }

    _kernel_setenv("PipeDream$Running", "Yes"); /* Set */

    /* Go and find something to do now -  just loop getting events until we are told to curl up and die */
    for(;;)
    {
        wm_events_get(FALSE); /* fg null events not wanted */
    }

    return(EXIT_SUCCESS);
}

/******************************************************************************
*
* called by atexit() on exit(rc)
*
******************************************************************************/

#if defined(APCS_SOFTPCS)
static void
application__atexit_apcs_softpcs(void)
{
    apcs_softpcs_finalise();
}
#endif

static void
application__atexit(void)
{
    _kernel_setenv("PipeDream$Running", NULL); /* Unset early in case anything subsequent barfs */

    font_close_all(TRUE); /* shut down font system releasing handles */

    reset_mc();

    riscos_finalise_once();

    file_finalise();

    riscos_hourglass_off();

/* On RISC OS don't bother to release the escape & event handlers
 * as the C runtime library restores the caller's handlers prior to exiting
 * to the calling program.
 * It also gives us a new ms more protection against SIGINT.
*/

/* On RISC OS wimpt__exit() is now called iff wimpt_init has been executed
 * as the RISC_OSLib library will have done an atexit() call too.
 *
 * The flex library may also have registered an atexit() handler
 * to free any allocated dynamic area.
*/
#ifdef PROFILING
    profile_off();

    profile_output(PROFILE_SORT_BY_COUNT, "RAM:pro_count", ",");
    profile_output(PROFILE_SORT_BY_ADDR,  "RAM:pro_addr",  NULL);
    profile_output(PROFILE_SORT_BY_NAME,  "RAM:pro_name",  "\t");
#endif
}

/******************************************************************************
*
*  exec pd.key if it exists, in its own little domain
*
******************************************************************************/

static void
exec_pd_key_once(void)
{
    char array[BUF_MAX_PATHSTRING];

    if(status_done(add_path_using_dir(array, elemof32(array), INITEXEC_STR, MACROS_SUBDIR_STR)))
        exec_file(array);
}

/* exec a macro file, using current document if one set, otherwise inventing one */

extern void
exec_file(
    const char *filename)
{
    P_DOCU p_docu = find_document_with_input_focus();

    if(NO_DOCUMENT == p_docu)
    {   /* The Filer may have just stolen the caret from us when the user double-clicked! */
        if(HOST_WND_NONE != caret_stolen_from_window_handle)
            p_docu = find_document_using_window_handle(caret_stolen_from_window_handle);
    }

    if(NO_DOCUMENT != p_docu)
    {
        /*>>>RCM asks, what should we do if xf_inexpression or xf_inexpression_box is true???*/
        DOCNO old_docno = current_docno();

        select_document(p_docu);

        do_execfile((char *) filename);

        if(is_current_document())
        {
            draw_screen();
            draw_caret();
        }

        select_document_using_docno(old_docno);

        return;
    }

    if(create_new_untitled_document())
    {
        DOCNO exec_docno = current_docno();
        BOOL resetwindow = TRUE;

        /* bring window to front NOW so that we can see
         * what's happening in the exec file
        */
        riscos_front_document_window(TRUE);

        draw_screen();
        draw_caret();

        do_execfile((char *) filename);

        if(is_current_document())
        {
            (void) mergebuf_nocheck();

            if(xf_filealtered)          /* watch out for strange people */
            {
                draw_screen();
                draw_caret();
                if(exec_docno == current_docno())
                    resetwindow = FALSE;
            }
        }

        /* try to delete the one we started at */
        if(!is_current_document()  ||  (exec_docno != current_docno()))
        {
            p_docu = p_docu_from_docno(exec_docno);

            if(p_docu != NO_DOCUMENT)
            {
                assert(!docu_is_thunk(p_docu));

                select_document(p_docu);

                (void) mergebuf_nocheck();

                if(xf_filealtered)          /* watch out for strange people */
                {
                    draw_screen();
                    draw_caret();
                    resetwindow = FALSE;
                }
                else
                    destroy_current_document();
            }
        }

        if(resetwindow)
            riscos_resetwindowpos();
    }
}

/******************************************************************************
*
* Functions to return basic information about product
*
******************************************************************************/

_Check_return_
_Ret_z_
extern PC_USTR
product_id(void)
{
    return(product_name);
}

_Check_return_
_Ret_z_
extern PC_USTR
product_ui_id(void)
{
    return(product_ui_name);
}

_Check_return_
_Ret_z_
extern PCTSTR
registration_number(void)
{
    static TCHARZ visRegistrationNumber[] = TEXT("0000 0000 0000 0000"); /* NB spaces */

    UINT grpidx;

    if(CH_NULL != __registration_number[0])
    {
        for(grpidx = 0; grpidx < 4; grpidx++)
        {
            /* copy groups of four over */
            memcpy32(&visRegistrationNumber[grpidx * 5], &__registration_number[grpidx * 4], 4 * sizeof(TCHAR));
        }
    }

    return(visRegistrationNumber);
}

_Check_return_
_Ret_z_
extern PCTSTR
user_id(void)
{
    return(__user_name);
}

_Check_return_
_Ret_z_
extern PCTSTR
user_organ_id(void)
{
    return(__organisation_name);
}

/*ncr*/
_Ret_z_
extern PTSTR
make_var_name(
    _Out_writes_z_(elemof_buffer) PTSTR var_name /*filled*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PCTSTR suffix)
{
    tstr_xstrkpy(var_name, elemof_buffer, product_id());
    tstr_xstrkat(var_name, elemof_buffer, suffix);
    return(var_name);
}

/*
no longer does anything on pass 1
*/

static void
decode_command_line_options(
    _InVal_     int argc,
    char * argv[],
    _InVal_     int pass)
{
    int i;

    for(i = 1; i < argc; ++i)
    {
        char *arg = argv[i];
        char ch   = *arg;
        char array[BUF_MAX_PATHSTRING];

        // if(pass == 2)
        //    reportf("main: *** got arg %d, '%s'", i, arg);

        switch(ch)
        {
        case '-':
            ch = *++arg;
            ch = (char) tolower(ch);
            switch(ch)
            {
            case 'c': /* Command */
            case 'm': /* Macro (for compatibility) */
                arg = argv[++i];
                if(pass == 2)
                    if(status_done(add_path_using_dir(array, elemof32(array), arg, MACROS_SUBDIR_STR)))
                        exec_file(arg);
                break;

            case 'h': /* Help */
                if(pass == 2)
                    application_process_command(N_Help);
                break;

            case 'k':
                arg = argv[++i];
                if(pass == 2)
                    thicken_grid_lines(atoi(arg));
                break;

            case 'l': /* Locale */
                arg = argv[++i];
                if(pass == 2)
                {   /* set the specified locale - if that fails, fall back to 'C' locale */
                    if(NULL != setlocale(LC_ALL, arg))
                        reportf("setlocale(%s) => %s", arg, setlocale(LC_ALL, NULL));
                    else
                        consume_ptr(setlocale(LC_ALL, "C"));
                }
                break;

            case 'n': /* New */
                if(pass == 2)
                    if(create_new_untitled_document())
                    {
                        /* don't acquire caret here, done later */
                        /* bring to front sometime */
                        xf_front_document_window = TRUE;
                        draw_screen();
                    }
                break;

            case 'p': /* Print */
                arg = argv[++i];
                if(pass == 2)
                    print_file(arg);
                break;

            case 'q': /* Quit */
                if(pass == 2)
                    application_process_command(N_Quit);
                break;

            default:
                messagef(Unrecognised_arg_Zs_STR, arg);
                break;
            }
            break;

        default:
            if(pass == 2)
            {
                LOAD_FILE_OPTIONS load_file_options;
                load_file_options_init(&load_file_options, arg, AUTO_CHAR /* therefore must go via loadfile() */);
                (void) loadfile(arg, &load_file_options);
            }
            break;
        }   /* esac */
    }   /* od */
}

static void
decode_run_options(void)
{
    char var_name[BUF_MAX_PATHSTRING];
    char run_options[BUF_MAX_PATHSTRING];
    const char * p_u8 = run_options;

    if(NULL != _kernel_getenv(make_var_name(var_name, elemof32(var_name), "$RunOptions"), run_options, elemof32(run_options)))
        return;

    while(NULL != p_u8)
    {
        const char * arg = p_u8;

        StrSkipSpaces(arg); /* skip leading spaces */

        if(CH_NULL == *arg)
            break;

        // reportf("main: *** got run option arg '%s'", arg);

        if((*arg == '-')  && (arg[1] == '-'))
        {
            U32 arg_len;
            PC_U8Z test_arg;
            U32 test_len;

            arg += 2; /* skip -- */
            arg_len = strlen32(arg);

            test_arg = "dynamic-area-limit=";
            test_len = strlen32(test_arg);

            if((arg_len >= test_len) && (0 == memcmp(arg, test_arg, test_len)))
            {
                P_U8Z p_u8_skip;
                U32 u32;

                arg += test_len;

                u32 = fast_strtoul(arg, &p_u8_skip);

                if(u32 != U32_MAX)
                    g_dynamic_area_limit = (u32 * 1024) * 1024; /*MB*/

                arg = p_u8_skip; /* skip digits */
            }
        }
        else
        {
            ++arg;
        }

        p_u8 = strchr(arg, SPACE); /* point past this arg, may end with NULL */
    }
}

static void
host_initialise_file_path(void)
{
    TCHARZ resource_path[BUF_MAX_PATHSTRING * 4];

    /* _kernel_getenv() expands the path even if SetMacro used - helps when supporting documents are loaded via the path */
    if(NULL == _kernel_getenv("PipeDream$Path", resource_path, elemof32(resource_path)))
        file_set_path(resource_path);
}

/* end of pdmain.c */
