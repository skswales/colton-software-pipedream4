/* pdmain.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

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

#include "riscos_x.h"
#include "ridialog.h"
#include "pd_x.h"

#include "cmodules/wm_event.h"

#ifndef __swis_h
#include "swis.h" /* C: */
#endif

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
static U8 user_name[32];
static U8 organ_name[32];

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

static void
get_user_info(void)
{
    U8 env_value[BUF_MAX_PATHSTRING];
    U8 var_name[BUF_MAX_PATHSTRING];

    make_var_name(var_name, elemof32(var_name), "$User1");
    if(NULL == _kernel_getenv(var_name, env_value, elemof32(env_value)))
        if(0 != strcmp(env_value, "NoInfo"))
            xstrkpy(user_name, elemof32(user_name), env_value);

    make_var_name(var_name, elemof32(var_name), "$User2");
    if(NULL == _kernel_getenv(var_name, env_value, elemof32(env_value)))
        xstrkpy(organ_name, elemof32(organ_name), env_value); /* SKS 13.12.98 */
}

/******************************************************************************
*
* Application starts here
*
******************************************************************************/

/* The list of messages that this application is interested in */

static /*const*/ wimp_msgaction
pd_wimp_messages[] =
{
    wimp_MDATASAVE,
    wimp_MDATASAVEOK,
    wimp_MDATALOAD,
    wimp_MDATALOADOK,
    wimp_MDATAOPEN,
    wimp_MPREQUIT,
    wimp_PALETTECHANGE,
    wimp_SAVEDESK,
    wimp_MShutDown,

    wimp_MHELPREQUEST,

    Wimp_MPD_DDE,

    wimp_MMENUWARN,
    wimp_MMODECHANGE,
    wimp_MINITTASK,
    wimp_MMENUSDELETED,
    wimp_MWINDOWINFO,

    wimp_MPrintFile,
    wimp_MPrintSave,
    wimp_MPrintError,
    wimp_MPrintTypeOdd,

    wimp_MCLOSEDOWN /* terminate list with zero */
};

extern int
main(
    int argc,
    char * argv[])
{
    #if defined(NO_SURRENDER)
    /* ensure ESCAPE enabled for trapping */
    fx_x2(229, 0);
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

    wimpt_os_version_determine(); /* very early indeed */

    { /* allow application to run without any report info */
    char env_value[BUF_MAX_PATHSTRING];
    report_enable(NULL == _kernel_getenv("PipeDream$ReportEnable", env_value, elemof32(env_value)));
    } /* block */

#if TRACE_ALLOWED
    { /* allow trace version to run without any trace info */
    char env_value[BUF_MAX_PATHSTRING];
    if(NULL != _kernel_getenv("PipeDream$TraceEnable", env_value, elemof32(env_value)))
        trace_disable();
    else
    {
        _kernel_stack_chunk *scptr;
        unsigned long scsize;
        int val = atoi(env_value);
        if(0 != val)
        {
            trace_on();
            trace_1(TRACE_APP_PD4, "main: sp ~= " PTR_XTFMT, report_ptr_cast(&argc));
            scptr = _kernel_current_stack_chunk();
            scsize = scptr->sc_size;
            trace_3(TRACE_APP_PD4, "main: stack chunk " PTR_XTFMT ", size %lu, top " PTR_XTFMT, report_ptr_cast(scptr), scsize, report_ptr_cast((char *) scptr + scsize));
        }
    }
    } /* block */
#endif /* TRACE_ALLOWED */

    /* set locale for isalpha etc. (ctype.h functions) */
    trace_1(TRACE_OUT | TRACE_ANY, TEXT("main: initial CTYPE locale is %s"), setlocale(LC_CTYPE, NULL));
    consume_ptr(setlocale(LC_CTYPE, "C" /* standard ASCII NB NOT "ISO8859-1" (ISO Latin 1) */));

    get_user_info();

    decode_command_line_options(argc, argv, 1);

    decode_run_options();

    /* startup Window Manager interface */
    wimpt_set_spritename(product_spritename); /* For RISC OS 3.5 and later error reporting */
    wimpt_set_taskname(product_ui_id()); /* Optional; uses wimpt_init() parm when omitted */

    riscos_hourglass_on();

    /* keep world valid during initialisation */
    docu_array_init_once();

    /* need to have set up the resource place and the program name for all to work ok */
    res_init(product_id());  /* Resources are in <applicationname$Dir> */

    /* Startup Window Manager interface */
    wimpt_wimpversion(310);
    wimpt_messages(pd_wimp_messages);
    wimpt_init(de_const_cast(char *, product_ui_id()));

#ifdef PROFILING
    /* Task ID may now have changed, so re-register */
    profile_taskchange();
#endif

    if(status_fail(aligator_init()))
    {
        /* use base allocator to get error out */
        strings_init();
        reperr_init();

        reperr_fatal(reperr_getstr(create_error(ERR_NOTINDESKTOP)));
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
        xf_frontmainwindow = TRUE;
        draw_screen();
    }

    /* turn off null events unless we really want them - tested at the exit of each event processed */
    event_setmask((wimp_emask) (event_getmask() | wimp_EMNULL));

    /* set up a point we can longjmp back to on serious error */
    {
    static jmp_buf program_safepoint;

    if(setjmp(program_safepoint))
    {
        /* returned here from exception */

        /* SKS after 4.11 31jan92 - tidy up a little */
        riscos_printing = FALSE;
        alt_array[0] = NULLCH;
    }
    else
        wimpt_set_safepoint(&program_safepoint);
    }

    /* Go and find something to do now -  just loop getting events until we are told to curl up and die */
    for(;;)
        wm_events_get(FALSE); /* fg null events not wanted */

    return(EXIT_SUCCESS);
}

/******************************************************************************
*
* called by atexit() on exit(rc)
*
******************************************************************************/

static void
application__atexit(void)
{
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
 * as the CWimp library will have done an atexit() call too.
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

    if(add_path_using_dir(array, elemof32(array), INITEXEC_STR, MACROS_SUBDIR_STR))
        exec_file(array);
}

/* exec a macro file, using current document if one set, otherwise inventing one */

extern void
exec_file(
    const char *filename)
{
    P_DOCU p_docu;

    if(NO_DOCUMENT != (p_docu = find_document_with_input_focus()))
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
        riscos_frontmainwindow(TRUE);

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
extern PC_USTR
user_id(void)
{
    return(user_name);
}

_Check_return_
_Ret_z_
extern PC_USTR
user_organ_id(void)
{
    return(organ_name);
}

extern void
make_var_name(
    _Out_writes_z_(elemof_buffer) P_USTR var_name /*filled*/,
    _InVal_     U32 elemof_buffer,
    _In_z_      PC_USTR p_u8_suffix)
{
    xstrkpy(var_name, elemof_buffer, product_id());
    xstrkat(var_name, elemof_buffer, p_u8_suffix);
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

        if(pass == 2)
            reportf("main: *** got arg %d, '%s'", i, arg);

        switch(ch)
        {
        case '-':
            ch = *++arg;
            ch = tolower(ch);
            switch(ch)
            {
            case 'h':
                if(pass == 2)
                    application_process_command(N_Help);
                break;

            case 'k':
                arg = argv[++i];
                if(pass == 2)
                    thicken_grid_lines(atoi(arg));
                break;

            case 'l':
                arg = argv[++i];
                if(pass == 2)
                {
                    setlocale(LC_CTYPE, arg);
                }
                break;

            case 'm':
                arg = argv[++i];
                if(pass == 2)
                    if(add_path_using_dir(array, elemof32(array), arg, MACROS_SUBDIR_STR))
                        exec_file(arg);
                break;

            case 'n':
                if(pass == 2)
                    if(create_new_untitled_document())
                    {
                        /* don't acquire caret here, done later */
                        /* bring to front sometime */
                        xf_frontmainwindow = TRUE;
                        draw_screen();
                    }
                break;

            case 'p':
                arg = argv[++i];
                if(pass == 2)
                    print_file(arg);
                break;

            case 'q':
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
                zero_struct(load_file_options);
                load_file_options.document_name = arg;
                load_file_options.filetype_option = AUTO_CHAR; /* therefore must go via loadfile() */
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

    make_var_name(var_name, elemof32(var_name), "$RunOptions");
    if(NULL != _kernel_getenv(var_name, run_options, elemof32(run_options)))
        return;

    while(NULL != p_u8)
    {
        const char * arg = p_u8;

        /* skip leading spaces */
        while(SPACE == *arg)
            ++arg;

        if(NULLCH == *arg)
            break;

        reportf("main: *** got run option arg '%s'", arg);

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

#if 0
    _kernel_swi_regs r;
    r.r[0] = (int) "PipeDream$Path";
    r.r[1] = (int) resource_path;
    r.r[2] = elemof32(resource_path) - 1;
    r.r[3] = 0;
    r.r[4] = 0; /* don't expand here in case user really does want SetMacro */
    if(NULL == _kernel_swi(OS_ReadVarVal, &r, &r))
    {
        resource_path[r.r[2]] = NULLCH;
        file_set_path(resource_path);
    }
#else
    if(NULL == _kernel_getenv("PipeDream$Path", resource_path, elemof32(resource_path)))
        file_set_path(resource_path);
#endif
}

/* end of pdmain.c */
