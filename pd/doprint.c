/* doprint.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Module that does printing from PipeDream */

/* RJM August 1987 */

#include "common/gflags.h"

#include "datafmt.h"

#ifndef __swis_h
#include "swis.h" /*C:*/
#endif

#ifndef __os_h
#include "os.h"
#endif

#ifndef __cs_bbcx_h
#include "cs-bbcx.h"
#endif

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"  /* includes wimpt.h -> wimp.h */
#endif

#ifndef __cs_flex_h
#include "cs-flex.h"    /* includes flex.h */
#endif

#ifndef __drawmod_h
#include "drawmod.h"
#endif

#include "riscos_x.h"
#include "riscdraw.h"

/*
internal functions
*/

static coord
header_footer_length(
    COL first,
    COL last);

static void
off_highlights(void);

static void
out_h_string_value_0(
    PC_LIST lptr);

static S32
printx(void);

#define print_newline() putchar('\n')

static void
prncan(
    BOOL ok);

static void
prnoff(
    BOOL ok);

static S32
prnon(void);

static void
rawprint(
    _InVal_     U8 ch);

static void
setpvc(void);

static void
finalise_saved_print_state(void);

static void
initialise_saved_print_state(void);

extern DIALOG d_edit_driver[];

/******************************************************************************
* print routines
******************************************************************************/

static S32 h_on = 0;       /* 8 bits specifying which highlights are on */

static BOOL out_h_on = FALSE;   /* output PON highlight string? */
static BOOL out_h_off = FALSE;  /* output POFF highlight string */

static BOOL escape_pressed = FALSE;

static S32 riscos_copies;
static S32 riscos_scale;
static S32 riscos_seqnum;

/* range of cells to print */
static SLR printing_first;
static SLR printing_last;

#define MACROFILE_BUFSIZ 1024
static FILE_HANDLE macrofile = NULL;        /* input TAB parameter file */
static void *macro_buffer = NULL;           /* where it's buffered */

static S32         printer_output_error = 0;
static FILE_HANDLE printer_output       = NULL; /* where print and spooling goes */
static FILE_HANDLE saved_printer_output = NULL; /* hold print stream while missing
                                                 * page in sheets */

static BOOL had_top;                        /* printed top of page */
static BOOL reset_pnm;                      /* was 'new file loaded' */
static S32 pagcnt;                          /* lines printed on page so far not including margins */

static BOOL sheets_bit;                     /* pause between pages */
static BOOL screen_bit;                     /* print to screen */
static BOOL print_failure;
static BOOL allow_print;

static coord linessofar;        /* number of lines draw on screen in screen command */
static S32 prnspc = 0;          /* spaces waiting to be printed */

coord header_footer_width;      /* header,footer width */

/* a printer driver is an array of output routines */

/* prnvec is pointer to printer driver */
static DRIVER * prnvec = BAD_POINTER_X(DRIVER *, 0);

static BOOL drvout(_InVal_ U8 ch);
static BOOL drvoff(BOOL ok);
static BOOL drvon(void);

static DRIVER loaded_driver =
{
    drvout,
    drvon,
    drvoff
};

static BOOL defout(U8 ch);

static DRIVER default_driver =
{
    defout,
    drvon,
    drvoff
};

static BOOL riscos_drvout(_InVal_ U8 ch);
static BOOL riscos_drvon(void);
static BOOL riscos_drvoff(BOOL ok);

static DRIVER riscos_driver =
{
    riscos_drvout,
    riscos_drvon,
    riscos_drvoff
};

#define block_option (d_print_QB == 'Y')

#define two_sided (d_print_QT == 'Y')
static coord two_sided_margin;

#define left_page (two_sided && ((curpnm & 1) == 0))

#define landscape_option (d_print_QL == 'L')

#define baseline_offset ((global_font_leading_mp * 1) / 8)

/* ----------------------------------------------------------------------- */

#if RISCOS && TRACE_ALLOWED

static void
riscos_printing_abort(
    const char *procname)
{
    if(riscos_printing)
    {
        prncan(FALSE);

        messagef("Procedure %s called during RISC OS printing", procname);
    }
}

#else
#define riscos_printing_abort(procname)
#endif

/******************************************************************************
*
* small module to initialise a serial
* printer on MSDOS or Archie
*
******************************************************************************/

static S32
init_serial_print(
    S32 port,
    S32 baudrate,
    S32 databits,
    S32 stopbits,
    S32 parity)
{
    S32 rs423config;

    IGNOREPARM(port);

    /* set default RS423 state */
    _kernel_osbyte(181, 1, 0);

    /* set baud rates */
    _kernel_osbyte(7, baudrate, 0);
    _kernel_osbyte(8, baudrate, 0);

    /* set RS423 configuration */
    if(parity & 1)
    {
        rs423config = ((databits & 1) << 4) |
                      ((stopbits & 1) << 3) |
                      (((parity >> 1) ^ 1) << 2);
    }
    else
        /* handle no parity case */
    {
        rs423config = (1 << 4) | (((stopbits & 1) ^ 1) << 2);
    }
    _kernel_osbyte(156, rs423config, 0xE3);

    return(1);
}

/******************************************************************************
*
* pause until SHIFT is pressed
*
******************************************************************************/

static S32
pause(void)
{
    BOOL tprnbit;
    S32 res;

    trace_0(TRACE_APP_PD4, "pause()");

    if(ctrlflag  ||  escape_pressed)
    {
        ack_esc();
        escape_pressed = TRUE;
        return(create_error(ERR_ESCAPE));
    }

    tprnbit = prnbit;
    prnoff(TRUE);

    escape_enable();

    clearkeyboardbuffer();

    while(!host_shift_pressed()  &&  !ctrlflag)
        ;

    if(escape_disable_nowinge())
        escape_pressed = TRUE;

    print_newline();

    if(tprnbit)
        res = prnon();
    else
        res = 1;

    linessofar = 0;

    return(res);
}

/******************************************************************************
*
* print the text to the printer
*
******************************************************************************/

extern void
Print_fn(void)
{
    uchar d_old_QL = d_print_QL;
    short d_old_QS = d_print_QS;
    S32   res = FALSE;

    if(dialog_box_start())
    {
        res = dialog_box(D_PRINT);

        /* never persists */
        dialog_box_end();
    }

    /* orientation change: redo lines */
    if((d_print_QL != d_old_QL) || (d_print_QS != d_old_QS))
        new_font_leading(global_font_leading_mp);

    if(res)
        print_document();
}

static _kernel_oserror *
swi_wimp_textcolour(int colour)
{
    _kernel_swi_regs rs;
    rs.r[0] = colour;
    return(_kernel_swi(/*TextColour*/ 0x000400F0, &rs, &rs));
}

/******************************************************************************
*
* MRJC created this fudger routine 9/8/89
* to fix problems with wait between pages
* and multiple file print
*
******************************************************************************/

static void
wait_vdu_reset(
    BOOL clear)
{
    if(sheets_bit || screen_bit)
    {
        wimpt_safe(bbc_vdu(bbc_TextToText));

        wimpt_safe(swi_wimp_textcolour(7));
        wimpt_safe(swi_wimp_textcolour(0 | 0x80));

        if(clear)
            wimpt_safe(bbc_vdu(bbc_ClearText));

        wimpt_safe(bbc_vdu(bbc_MoveText)); /* TAB x,y */
        wimpt_safe(bbc_vdu(0));
        wimpt_safe(bbc_vdu(scrnheight()));
    }
}

/*
find the last column that will fit in a given width
partial columns aren't allowed, unless they are first
*/

static COL
last_col_in_width(
    COL first_col,
    coord width)
{
    COL tcol;
    coord width_so_far = 0;
    COL this_fits = first_col;

    for(tcol = first_col; tcol < numcol; tcol++)
    {
        P_S32 widp, wwidp;

        readpcolvars(tcol, &widp, &wwidp);

        if(*widp > 0)
        {
            width_so_far += *widp;

            /* if blown off end send back previous one */
            if(width_so_far > width)
                return(this_fits);
            else if(width_so_far == width)
                return(tcol);

            this_fits = tcol;
        }
    }

    return(this_fits);
}

static char *printing_macro_buffer = NULL;

static S32
print_document_core_core(
    const char ** errp,
    _OutRef_    P_S32 cancan)
{
    char   parmfile_array[BUF_MAX_PATHSTRING];
    char * parmfile;
    S32    copies, res;

    *cancan = FALSE;

    /* do we have mailshot parameters? */
    if(d_print_QD == 'Y')
    {
        parmfile = d_print_QE;

        if(parmfile)
        {
            res = file_find_on_path_or_relative(parmfile_array, elemof32(parmfile_array), parmfile, currentfilename)
                        /* doing macros - check it's a tab file */
                        ? find_filetype_option(parmfile_array, FILETYPE_UNDETERMINED)
                        : '\0';

            if(res != 'T')
            {
                res = (res == '\0') ? create_error(ERR_NOTFOUND) : create_error(ERR_NOTTABFILE);
                /* must pass back a pointer to a non-auto object */
                *errp = parmfile;
                return(res);
            }

            macrofile = pd_file_open(parmfile_array, file_open_read);

            if(macrofile)
            {
                STATUS status;
                if(NULL != (printing_macro_buffer = al_ptr_alloc_bytes(char *, MACROFILE_BUFSIZ, &status)))
                    file_buffer(macrofile, printing_macro_buffer, MACROFILE_BUFSIZ);
            }
            else
            {
                *errp = parmfile;
                return(create_error(ERR_NOTFOUND));
            }
        }
    }
    else
        parmfile = NULL;

    copies = d_print_QN;

    /* RISC OS printing does multiple copies differently */
    if(riscos_printing)
    {
        riscos_copies = copies;
        copies = 1;
        riscos_scale  = d_print_QS;
    }

    draw_to_screen = FALSE;

    res = 0;

    if(copies)
    {
        initialise_saved_print_state();

        do  {
            BOOL triscos_fonts;

            if(macrofile)
                file_seek(macrofile, 0, SEEK_SET);

            sqobit = TRUE;
            trace_0(TRACE_APP_PD4, "had_top := FALSE");
            had_top = FALSE;

            res = prnon();

            /* never call prncan if prnon not been called */
            *cancan = TRUE;

            if(res <= 0)
                break;

            triscos_fonts = font_stack(riscos_fonts);

            if(!riscos_printing)
                riscos_fonts = FALSE;

            res = printx();

            if(is_current_document())
                riscos_fonts = font_unstack(triscos_fonts);

            if(screen_bit)
                (void) pause();

            if(res <= 0)
                break;
        }
        while(--copies > 0);

        finalise_saved_print_state();
    }

    /* close macro file */
    if(macrofile)
    {
        pd_file_close(&macrofile);
        al_ptr_dispose(P_P_ANY_PEDANTIC(&macro_buffer));
    }

    return(res);
}

static S32
page_width_query(void)
{
    /* SKS after 4.11 06jan92 - add paranoid checks on page_width; made function as it's now used much more */
    S32 page_width;

    page_width = d_poptions_PX;
    page_width = (page_width > d_poptions_LM)    ? (page_width - d_poptions_LM)    : 0;
    page_width = (page_width > two_sided_margin) ? (page_width - two_sided_margin) : 0;

    return(page_width);
}

static char outfile_array[BUF_MAX_PATHSTRING]; /* so we can pass back a pointer to it */

static S32
print_document_core(
    const char ** errp)
{
    const char * outname    = NULL;
    const char * outnamerep = NULL;
    BOOL mustredraw = FALSE;
    S32 cancan = FALSE;
    S32 res;

    *errp = NULL;

    currently_inverted = currently_protected = FALSE;

    ack_esc();
    escape_pressed = FALSE;

    allow_print = TRUE;
    print_failure = FALSE;
    riscos_printing = FALSE;

    screen_bit = FALSE;
    sheets_bit = (d_print_QW == 'Y');

    if(sheets_bit  &&  (d_poptions_PL == 0))
        return(create_error(ERR_NOPAGES));

    /* if block specified, check block marked */
    if(block_option  &&  !MARKER_DEFINED())
        return(MARKER_SOMEWHERE() ? create_error(ERR_NOBLOCKINDOC) : create_error(ERR_NOBLOCK));

    /* setup printer destination */
    switch(d_print_QP)
    {
    case 'F':
        {
        /* it's going to a file */
        outnamerep = d_print_QF;

        if(str_isblank(outnamerep))
            return(create_error(ERR_BAD_NAME));

        (void) file_add_prefix_to_name(outfile_array, elemof32(outfile_array), outnamerep, currentfilename);

        outname = outfile_array;

        if(!checkoverwrite(outname))
            return(0);

        sheets_bit = FALSE;
        }
        break;

    /* it's going to the screen */
    case 'S':
        {
        outname = "rawvdu:"; /* unfortunately vdu: filters tbs chars */

        screen_bit = TRUE;
        sheets_bit = FALSE;
        linessofar = 0;
        }
        break;

    /* it's going to printer */
    default:
        {
        trace_1(TRACE_APP_PD4, "outputting to driver type %d", d_driver_PT);

        switch(d_driver_PT)
        {
        default:
        case driver_riscos:
            riscos_printing = TRUE;
            outname = "rawvdu:"; /* still need for output when waiting between pages */
            break;

        case driver_serial:
            init_serial_print(0, /*port*/
                              d_driver_PB,       /*baudrate*/
                              d_driver_PW - '5', /*databits*/
                              d_driver_PO - '1', /*stopbits*/
                              (d_driver_PP == 'N') ? 0 :
                              (d_driver_PP == 'O') ? 1 : 2   /*parity*/);

            outname = "printer#serial:";
            break;

        case driver_parallel:
            outname = "printer#parallel:";
            break;

        case driver_network:
            if(!str_isblank(d_driver_PN))
            {
                /* SKS after 4.11 06jan92 - $.Stream is bodge for netprint 5.23 on RISC OS 3 */
                (void) xsnprintf(outfile_array, elemof32(outfile_array), "netprint#%s:$.Stream", d_driver_PN);
                outname = outfile_array;
            }
            else
                outname = "netprint:";
            break;

        case driver_user:
            outname = "printer#user:";
            break;
        }
        }
        break;
    }

    printer_output_error = 0;
    printer_output       = NULL;
    saved_printer_output = NULL;

    assert(outname);
    if(outname)
    {
        escape_enable();
        printer_output = pd_file_open(outname, file_open_write);
        escape_disable();

        if(!printer_output)
        {
            *errp = outnamerep ? outnamerep : outname;
            riscos_printing = FALSE; /* hole closed by SKS after 4.11 17jan92 */
            return(create_error(ERR_CANNOTOPEN));
        }

        /* ensure unbuffered */
        file_buffer(printer_output, NULL, 0);

        if(d_print_QP == 'F')
        {
            /* Make real (user-specified) file a 'Printout' type file so we can
             * drag it to the PrinterXX application to print in the background
            */
            escape_enable();
            file_set_type(printer_output, FILETYPE_PRINTOUT);
            escape_disable();
        }
    }

    { /* SKS 27jan97 add some headroom for printing */
    void * blkp;
    STATUS status;

    if(NULL == (blkp = _al_ptr_alloc(32*1024, &status)))
        return(status);

    flex_forbid_shrink(TRUE);

    al_ptr_dispose(&blkp);
    }

    two_sided_margin = (!two_sided  ||  str_isblank(d_print_QM))
                                ? 0
                                : atoi(d_print_QM);

    if(sheets_bit  ||  screen_bit)
    {
        wait_vdu_reset(TRUE);
        mustredraw = TRUE;

        print_newline();

        if(sheets_bit)
            puts(Initialising_STR);
    }

    /* output PON as soon as printing starts */
    out_h_on = TRUE;

    /* if page width not set to 0, print page by page across the document/marked block */
    if(d_poptions_PX == 0)
        res = print_document_core_core(errp, &cancan);
    else
    {
        /* print only as many column across as will fit in page width */
        /* set up block for each set of columns and print it */

        SLR save_bs, save_be, use_bs, use_be;
        DOCNO save_bd;
        COL new_column;
        S32 page_width;
        uchar save_QB;

        save_bs = blkstart;
        save_be = blkend;
        save_bd = blk_docno;

        if(block_option)
        {
            use_bs = blkstart;
            use_be = blkend;
        }
        else
        {
            use_bs.col = 0;
            use_bs.row = 0;
            use_be.col = numcol-1;
            use_be.row = numrow-1;
        }

        blk_docno = current_docno();

        save_QB    = d_print_QB;
        d_print_QB = 'Y'; /* pretend to be printing blocks */

        /* Dan thinks subtracting the left margin from the page width is correct */
        page_width = page_width_query();

        res = 0;

        new_column = use_bs.col;

        while(new_column <= use_be.col)
        {
            /* start at this column and row */
            blkstart.col = new_column;
            blkstart.row = use_bs.row;

            /* see if this and ALL columns to the right
             * in the block/doc to be printed are empty
             * stop print loop if so
            */
            if(is_blank_block(blkstart.col, blkstart.row,
                              use_be.col,   use_be.row  ))
                break;

            /* determine end column from page width and use_be limit */
            blkend.col   = last_col_in_width(new_column, page_width);
            /* SKS after 4.11 06jan92 - don't exceed specified last column! */
            blkend.col   = MIN(blkend.col, use_be.col);
            blkend.row   = use_be.row;

            res = print_document_core_core(errp, &cancan);
            if(res < 0)
                break;

            /* start here next time round */
            new_column = blkend.col + 1;
        }

        d_print_QB   = save_QB;

        blkstart  = save_bs;
        blkend    = save_be;
        blk_docno = save_bd;
    }

    /* output POFF after all printing done but only if prnon has been called in print_document_core_core */
    if(cancan)
        prncan(TRUE);

    if(printer_output || saved_printer_output) /* could be in either place, the latter is more likely and desired */
    {
        char * err;
        S32    res1;

        if(saved_printer_output)
        {
            assert(!printer_output); /* out of sync? */
            printer_output = saved_printer_output;
        }

        escape_enable();

        if(!printer_output_error)
        {
            (void) file_get_error();

            if((res1 = file_flush(printer_output)) < 0)
            {
                res = (res <= 0) ? res : res1;

                err = file_get_error();

                if(err)
                    rep_fserr(err);
                else
                    reperr(res, d_print_QF);
            }
        }

        trace_0(TRACE_APP_PD4, "closing printer_output");
        pd_file_close(&printer_output);

        escape_disable();
    }

    flex_forbid_shrink(FALSE);

    riscos_printing = FALSE;
    draw_to_screen = TRUE;

    if(mustredraw)
        /* we blew everyone's windows away */
        wimpt_forceredraw();

    if(print_failure)
        res = create_error(ERR_GENFAIL);

    return(res);
}

extern void
print_document(void)
{
    const char *errp;
    S32 res;

    /* RJM adds on 22.9.91 */
    load_driver(&d_driver_PD, FALSE);

    /* ensure completely recalced */
    ev_recalc_all();

    res = print_document_core(&errp);

    if(res < 0)
        reperr(res, errp);
}

/******************************************************************************
*
* set the pitch to n units
*
* NOP if RISC OS printing
*
******************************************************************************/

extern void
set_pitch(
    S32 n)
{
    char buffer[32];
    char *ptr;
    char ch;
    S32 offset;
    PC_LIST lptr;

    if(riscos_printing  ||  !prnbit  ||  !micbit)
        return;

    /* send prefix code */

    lptr = search_list(&highlight_list, HMI_P);
    if(!lptr)
        return;

    out_h_string_value_0(lptr);

    /* is there an offset ? */
    lptr = search_list(&highlight_list, HMI_O);
    if(lptr)
    {
        offset = (S32) *lptr->value;

        if(offset > 128)
            n -= (256 - offset);
        else
            n += offset;
    }

    /* send length of gap as byte or text form? */
    if(hmi_as_text)
    {
        /* send length of gap as text form */
        (void) sprintf(buffer, "%d", n);
        ptr = buffer;
        while((ch = *ptr++) != '\0')
            rawprint(ch);
    }
    else
        /* send length of gap as byte */
        rawprint(n);

    /* send suffix if it has one */
    out_h_string_value_0(search_list(&highlight_list, HMI_S));
}

static void
at_print(void)
{
    /* system font printing pos at top of char */
    /* + extra correction for system font baseline */
    print_complain(bbc_move(riscos_font_xad/MILLIPOINTS_PER_OS,
                            riscos_font_yad/MILLIPOINTS_PER_OS
                            + (charheight*(7-1))/8));
}

/******************************************************************************
*
* output a gap n microspace units long
*
******************************************************************************/

extern void
mspace(
    S32 n)
{
    trace_1(TRACE_APP_PD4, "mspace(%d)", n);

    if(riscos_printing)
    {
        print_complain(os_plot(bbc_MoveCursorRel, n, 0));
        return;
    }

    set_pitch(n);

    /* send gap */
    prnout(SPACE);

    /* reset to standard pitch */
    set_pitch(smispc);
}

/******************************************************************************
*
* update page number from 'new file'
*
******************************************************************************/

static void
getfpn(void)
{
    if(encpln == 0)
    {
        curpnm = 0;
        trace_1(TRACE_APP_PD4, "getfpn: curpnm := %d (encpln == 0)", curpnm);
    }
    else if(reset_pnm)
    {
        curpnm = filpnm;
        trace_1(TRACE_APP_PD4, "[getfpn: curpnm := %d (filpnm)]", curpnm);
    }

    reset_pnm = FALSE;
}

/* move print head down a number of lines */

static void
drop_n_lines(
    S32 nlines)
{
    trace_1(TRACE_APP_PD4, "drop_n_lines(%d)", nlines);

    if(nlines > 0)
    {
        if(riscos_printing)
        {
            /* drop baseline n lines */
            riscos_font_yad -= nlines * global_font_leading_mp;
            trace_3(TRACE_APP_PD4, "riscos_font_yad = %d (mp) after dropping %d line%s",
                    riscos_font_yad, nlines, (nlines == 1) ? "" : "s");
            return;
        }

        wrchrep(CR, nlines);
    }
}

extern S32
left_margin_width(void)
{
    S32 nspaces;

    nspaces = d_poptions_LM + ((left_page) ? 0 : two_sided_margin);

    trace_1(TRACE_APP_PD4, "left_margin_width() gives %d spaces", nspaces);

    return(nspaces);
}

static S32
print_left_margin(void)
{
    S32 nspaces;

    nspaces = left_margin_width();

    if(riscos_printing)
    {
        /* left margin and two-sided margins now accounted for in RISC OS printing rectangle */
        riscos_font_xad = 0;
    }
    else
        ospca(nspaces);

    return(nspaces);
}

/******************************************************************************
*
* process header or footer
*
******************************************************************************/

static void
prchef(
    char *field)
{
    char array[PAINT_STRSIZ];

    trace_2(TRACE_APP_PD4, "prchef(%s), page number = %d", trace_string(field), curpnm);

    /* SKS after 4.12 21apr92 - must test header/footer even on page 0 */
    if(str_isblank(field))
        return;

    if(curpnm)
    {
        /* set pitch if microspacing */
        setssp();
        set_pitch(smispc);

        print_left_margin();

        if(riscos_printing)
        {
            print_setcolours(FORE, BACK);

            if(riscos_fonts)
                print_setfontcolours();
            else
                at_print();
        }

        /* expand the lcr field and send it off */
        expand_lcr(field, -1, array, header_footer_width,
                   DEFAULT_EXPAND_REFS /*expand_refs*/, TRUE /*expand_ats*/, TRUE /*expand_ctrl*/,
                   riscos_fonts /*allow_fonty_result*/, TRUE /*compile_lcr*/);

        lcrjust(array, header_footer_width, left_page);
    }

    prnout(EOS);        /* switch off highlights */
    prnout(CR);
}

/******************************************************************************
*
* eject top of page
*
******************************************************************************/

static drawmod_line printing_draw_line_style =
{
    /* flatness */          0,
    /* thickness */         0,
    /* spec */              {
    /* spec.join */             join_mitred,
    /* spec.leadcap */          cap_butt,
    /* spec.trailcap */         cap_butt,
    /* spec.reserved8 */        0,
    /* spec.mitrelimit */       0x000A0000,
    /* spec.lead_tricap_w */    0,
    /* spec.lead_tricap_h */    0,
    /* spec.trail_tricap_w */   0,
    /* spec.trail_tricap_h */   0
                            },
    /* dash_pattern */      NULL
};

extern void
thicken_grid_lines(
    S32 thickness)
{
    printing_draw_line_style.thickness = thickness;
}

static void
print_grid_line(
    S32 x_ch,
    S32 x_across_ch,
    S32 y_row,
    S32 y_drop_rows)
{
    /* OS units */
    S32 x0, y0, x1, y1;
    int path[4*3 + 2];
    int * ptr = path;
    drawmod_pathelemptr admit_defeat;

    admit_defeat.wordp = path;

    /* starting x Draw - assumes print_left_margin sets riscos_font_xad to some small epsilon > 0 */
    x0 = x_ch * charwidth * 256 + (MILLIPOINTS_PER_OS / 4); /* SKS 31.10.91 / 16 works for landscape but not portrait */

    /* ending x Draw - across n chars from start */
    x1 = x0 + x_across_ch * charwidth * 256;

    /* starting y Draw - relative to current output position */
    y0 = (riscos_font_yad - y_row * global_font_leading_mp + global_font_leading_mp*3/4) * 256 / MILLIPOINTS_PER_OS;

    /* ending y Draw - drop n lines from start */
    y1 = y0 - (y_drop_rows * global_font_leading_mp) * 256 / MILLIPOINTS_PER_OS;

    *ptr++ = path_move_2;       /* MoveTo */
    *ptr++ = x0;
    *ptr++ = y0;

    *ptr++ = path_lineto;       /* DrawTo */
    *ptr++ = x1;
    *ptr++ = y1;

    *ptr++ = path_closeline;    /* EndOfPath */

    *ptr++ = path_term;         /* EndOfObject */

    print_setcolours(FORE, BACK);

    print_complain(drawmod_stroke(admit_defeat, fill_Default, NULL, &printing_draw_line_style));
}

static void
print_grid_lines(void)
{
    COL col;
    S32 rowoff, nrows; /* not actually rows when enclns > 1, they are lines */
    coord this_colstart, colwid;

    if(riscos_printing  &&  grid_on)
    {
        nrows = (printing_last.row - in_block.row + 1) * enclns; /* SKS 20130701 *= enclns */
        if( nrows > encpln) /* clip to page core */
            nrows = encpln;

        this_colstart = 0;

        /* draw initial vertical line */
        print_grid_line(0, 0, 0, nrows);

        /* draw subsequent right hand of each column lines */
        for(col = printing_first.col; col <= printing_last.col; ++col)
        {
            colwid = colwidth(col);

            if(colwid)
                print_grid_line(this_colstart, 0, 0, nrows);

            this_colstart += colwid;
        }

        /* draw final vertical line */
        print_grid_line(header_footer_width, 0, 0, nrows);

        /* draw first and subsequent horizontal lines across entire page */
        /* it may be that doing it at enclns is right. or maybe not */
        /* I'd say the latter if we did fonty rubout behind the printed text. so that's what it does */
        for(rowoff = 0; rowoff <= nrows; rowoff += enclns) /* SKS 20130701 += enclns */
            print_grid_line(0, header_footer_width, rowoff, 0);
    }
}

static BOOL
topejc(void)
{
    trace_0(TRACE_APP_PD4, "topejc()");

    getfpn();

    trace_0(TRACE_APP_PD4, "had_top := TRUE");
    had_top = TRUE;

    /* output top margin */
    drop_n_lines(d_poptions_TM);

    /* output header */
    prchef(d_poptions_HE);
    /* output header */
    prchef(d_poptions_H1);
    /* output header */
    prchef(d_poptions_H2);

    /* output header margin */
    drop_n_lines(d_poptions_HM);

    print_grid_lines();

    return(!been_error);
}

/******************************************************************************
*
* end of page string
*
* if the string doesn't contain a formfeed, only output if do_something
* is set, ie at the real end of page, not a trial one
*
* return TRUE if anything output
*
******************************************************************************/

static BOOL
outff(
    BOOL do_something)
{
    trace_1(TRACE_APP_PD4, "outff(%s)", trace_boolstring(do_something));

    if(riscos_printing)
        return(TRUE);

    if(!screen_bit)
    {
        PC_LIST lptr = search_list(&highlight_list, E_P);

        if(lptr)
        {
            /* we have string, can we output it? */

            if(do_something  ||  strchr((const char *) lptr->value, FORMFEED))
            {
                out_h_string_value_0(lptr);

                return(TRUE);
            }
        }
    }

    return(FALSE);
}

/******************************************************************************
*
* eject bottom of page
*
******************************************************************************/

static void
botejc(void)
{
    trace_0(TRACE_APP_PD4, "botejc()");

    trace_0(TRACE_APP_PD4, "had_top := FALSE");
    had_top = FALSE;

    /* if there are no footers and end of page string contains a FORMFEED,
     * just do end of page string.
     * check also we haven't output all lines on page
    */
    if(pagcnt  ||  (d_poptions_FM > 0)  ||  (d_poptions_BM > 0))
        if(str_isblank(d_poptions_FO) && str_isblank(d_poptions_F1) && str_isblank(d_poptions_F2))
            if(outff(FALSE))
                return;

    /* output lines to footer margin */
    if(pagcnt)
        drop_n_lines(real_pagelength - real_pagcnt);

    /* output footer margin */
    drop_n_lines(d_poptions_FM);

    /* output footer */
    prchef(d_poptions_FO);
    /* output footer */
    prchef(d_poptions_F1);
    /* output footer */
    prchef(d_poptions_F2);

    if(d_poptions_BM > 0)
    {
        /* if can do FORMFEED, do it */
        if(!outff(FALSE))
        {
            /* must output blanks to end of page */
            drop_n_lines(d_poptions_BM);

            outff(TRUE);
        }
    }
}

/*
*/

static BOOL
pagejc(void)
{
    trace_0(TRACE_APP_PD4, "pagejc()");

    if(!encpln)
        return(TRUE);

    if(had_top)
    {
        botejc();
        if(been_error)
            return(FALSE);
        getfpn();
        curpnm++;
        trace_1(TRACE_APP_PD4, "pagejc: ++curpnm := %d", curpnm);
        return(TRUE);
    }

    return(topejc());
}

/******************************************************************************
*
* read a TAB separated line from macrofile
* and stick parameters on list
*
******************************************************************************/

static S32
macrocall(void)
{
    char buffer[LIN_BUFSIZ];
    int res;
    S32 key = 0;
    S32 mres;

    trace_0(TRACE_APP_PD4, "macrocall()");

    if(!macrofile)
        return(0);

    /* delete old macro list */
    delete_list(&first_macro);

    for(;;)
    {
        res = getfield(macrofile, buffer, FALSE);

        switch(res)
        {
        case EOF:
            return(0);

        case CR:
        case TAB:
            if(!str_isblank(buffer))
                status_return(mres = add_to_list(&first_macro, key, buffer));
            /* omit blank fields ? */
            else if(d_print_QO == 'Y')
                --key;

            if(res == CR)
                return(1);
            break;

        default:
            break;
        }

        ++key;
    }
}

/* do once initially and once for each subsequent
 * file in a multi-file document print
*/

static void
init_print_selection(void)
{
    S32 page_width;

    trace_0(TRACE_APP_PD4, "*** init_print_selection");

    /* use marked block? */
    if(block_option)
    {
        printing_first = blkstart;
        printing_last  = (blkend.col == NO_COL) ? blkstart : blkend;

        /* ensure column range valid for this file */
        if(printing_last.col >= numcol - 1)
            printing_last.col = numcol - 1;
    }
    else
    {
        /* entire document? (all rows) - now only if glbbit || !d_poptions_PX */
        printing_first.col = (COL) 0;
        printing_first.row = (ROW) 0;
        printing_last.col  = numcol - 1;
        printing_last.row  = numrow - 1;
    }

    trace_4(TRACE_APP_PD4, "init_print_selection srow: %d, erow: %d, scol %d, ecol %d",
            printing_first.row, printing_last.row, printing_first.col, printing_last.col);

    trace_0(TRACE_APP_PD4, "init_print_selection checking last.col");

    /* find length of headers, footers */
    header_footer_width = header_footer_length(printing_first.col, printing_last.col + 1);

    /* obviously headers, footers need limiting to the page width! SKS 18.10.91 */
    page_width = page_width_query();

    if(page_width)
        header_footer_width = MIN(header_footer_width, page_width);

    trace_1(TRACE_APP_PD4, "init_print_selection header_footer_width: %d", header_footer_width);

    trace_0(TRACE_APP_PD4, "init_print_selection initing block");
    init_block(&printing_first, &printing_last);
}

/* print the page */

/* a huge graphics window for Draw file printing */
static GDI_BOX printing_draw_box = { -(1<<15), -(1<<15), (1<<15), (1<<15) };

#define P_HAD_ERROR 0
#define P_DONE_PAGE 1
#define P_FINISHED  2

static S32
print_page(void)
{
    /* for each file in multi-file document */
    for(;;)
    {
        coord this_colstart = 0;  /* keep dataflower happy */
        coord drawn_sofar   = 0;  /* only used for PD printing */
        ROW previous_row   = -1; /* an invalid row number */
        P_CELL tcell;
        coord fwidth, colwid, page_width;

        trace_0(TRACE_APP_PD4, "print_page loop");

        /* SKS after 4.11 06jan92 - see below */
        page_width = page_width_query();

        /* for each cell in file or block */
        while(next_in_block(ACROSS_ROWS))
        {
            trace_2(TRACE_APP_PD4, "print_page loop... col %d, row %d", in_block.col, in_block.row);

            /* see if user wants to stop */
            if(ctrlflag  ||  escape_pressed)
            {
                ack_esc();
                escape_pressed = TRUE;
                return(create_error(ERR_ESCAPE));
                }

            /* forced first time round */
            if(previous_row != in_block.row)
            {
                /* it's a new row so check for hard break */
                if(chkrpb(in_block.row))
                {
                    /* dont do hard break if coincides with soft break */
                    if( (in_block.row > 0)  &&  (pagcnt != enclns)  &&
                        chkpbs(in_block.row, pagcnt))
                    {
                        trace_0(TRACE_APP_PD4, "found active hard break line");

                        /* this code does not cater, like does VP, for
                         * hard/soft combinations and file boundaries
                        */
                        if(!pagejc())
                            return(P_HAD_ERROR);

                        pagcnt = enclns;
                        real_pagcnt = 0;

                        return(more_in_block(ACROSS_ROWS) ? P_DONE_PAGE : P_FINISHED);
                    }
                    #if TRACE_ALLOWED
                    else
                    {
                        trace_0(TRACE_APP_PD4, "found inactive hard break line");
                    }
                    #endif

                    force_next_row();
                    continue;       /* get another row to print */
                }

                previous_row = in_block.row;
                this_colstart = 0;
                drawn_sofar = 0;

                /* this seems to be user abandoning print before start */
                if(!had_top  &&  !pagejc())             /* prnrow1:  */
                    return(P_HAD_ERROR);

                getfpn();                               /* prnrow2: */

                /* set pitch if microspacing */
                setssp();
                set_pitch(smispc);
                print_left_margin();

                if(been_error)
                    return(P_HAD_ERROR);
            }

            colwid = colwidth(in_block.col);

            if(colwid)
            {
                /* update to start of column */
                if(!riscos_printing)
                    while(drawn_sofar < this_colstart)
                    {
                        prnout(SPACE);

                        if(been_error)
                            return(P_HAD_ERROR);

                        drawn_sofar++;
                    }

                /* draw cell */
                tcell = travel(in_block.col, in_block.row);

                /* get width of cell */
                fwidth = chkolp(tcell, in_block.col, in_block.row);

                if(page_width)
                {
                    /* SKS after 4.11 06jan92 - limit field widths to page */
                    colwid = MIN(colwid, page_width - this_colstart);
                    fwidth = MIN(fwidth, page_width - this_colstart);
                }

                if(tcell)
                {
                    P_DRAW_DIAG p_draw_diag;
                    P_DRAW_FILE_REF p_draw_file_ref;
                    S32 x, y;

                    if( riscos_printing  &&
                        draw_find_file(current_docno(), in_block.col, in_block.row, &p_draw_diag, &p_draw_file_ref))
                    {
                        /* pictures fill line, not just down from baseline */
                        trace_2(TRACE_APP_PD4, "found picture at %d, %d", in_block.col, in_block.row);

                        /* origin of draw file at x0, y1 */
                        x = riscos_font_xad / MILLIPOINTS_PER_OS;
                        y =(riscos_font_yad
                             + (global_font_leading_mp - baseline_offset)
                             - (p_draw_file_ref->y_size_os * MILLIPOINTS_PER_OS)
                            ) / MILLIPOINTS_PER_OS;

                        if(status_fail(draw_do_render(p_draw_diag->data, p_draw_diag->length, x, y, p_draw_file_ref->x_factor, p_draw_file_ref->y_factor, &printing_draw_box)))
                            /*print_complain(&err.err.os)*/;

                        /* Draw rendering has invalidated our colour cache */
                        killcolourcache();
                    }
                    else
                    {
                        trace_2(TRACE_APP_PD4, "printing cell %d, %d", in_block.col, in_block.row);

                        if((fwidth > colwid)  &&  !slot_displays_contents(tcell))
                            fwidth = colwid;

                        if(riscos_printing)
                        {
                            BOOL neg;

                            if(result_sign(tcell) < 0)
                                neg = TRUE;
                            else
                                neg = FALSE;

                            print_setcolours(neg ? NEGATIVEC : FORE, BACK);

                            if(riscos_fonts)
                                print_setfontcolours();
                            else
                                at_print();
                        }

                        drawn_sofar += outslt(tcell, in_block.row, fwidth);

                        prnout(EOS);
                    }
                }

                if(been_error)
                    return(P_HAD_ERROR);

                if(riscos_printing)
                    riscos_font_xad += colwid * (charwidth*MILLIPOINTS_PER_OS);

                this_colstart += colwid;
            }

            /* if at last column draw carriage returns */
            if(in_block.col == printing_last.col)
            {
                /* send out line spacing, checking not going over page break */
                S32 i;

                /* do some activity indicator using hourglass */
                actind_in_block(ACROSS_ROWS);

                for(i = 0; i < enclns; i++)
                {
                    if((real_pagcnt < real_pagelength)  ||  (real_pagelength == 0))
                    {
                        prnout(CR);
                        real_pagcnt++;
                    }

                    pagcnt++;

                    if((real_pagelength > 0)  &&  (real_pagcnt >= real_pagelength))
                    {
                        pagcnt = 0;
                        real_pagcnt = 0;
                        break;
                    }
                }

                if(pagcnt == 0)
                {
                    if(!pagejc())       /* bottom of page */
                        return(P_HAD_ERROR);

                    pagcnt = enclns;
                    real_pagcnt = 0;

                    return(more_in_block(ACROSS_ROWS) ? P_DONE_PAGE : P_FINISHED);
                }
            }
        }

        trace_0(TRACE_APP_PD4, "complete current page");

        /* do bottom of page */
        if(had_top  &&  !pagejc())
            return(P_HAD_ERROR);

        return(P_FINISHED);
    }
}

/* state to preserve - vague effort to reduce code on ARM */
typedef struct RISCOS_PRINT_SAVE_STR
{
    SLR saved_in_block;
    BOOL saved_start_block;
    S32 saved_pagcnt;
    S32 saved_real_pagcnt;
    BOOL saved_reset_pnm;
    S32 saved_curpnm;
    S32 saved_curfil;
    filepos_t saved_macrofilepos;
    P_LIST_BLOCK saved_macrolist;
    BOOL saved_had_top;
}
RISCOS_PRINT_SAVE_STR;

static RISCOS_PRINT_SAVE_STR riscos_print_save;

static void
initialise_saved_print_state(void)
{
    riscos_print_save.saved_macrolist = NULL;
}

static void
save_print_state(void)
{
    riscos_print_save.saved_in_block    = in_block;
    riscos_print_save.saved_start_block = start_block;
    riscos_print_save.saved_pagcnt      = pagcnt;
    riscos_print_save.saved_real_pagcnt = real_pagcnt;
    riscos_print_save.saved_reset_pnm   = reset_pnm;
    riscos_print_save.saved_curpnm      = curpnm;
    if(macrofile)
    {
        file_getpos(macrofile, &riscos_print_save.saved_macrofilepos);

        delete_list(&riscos_print_save.saved_macrolist);
        riscos_print_save.saved_macrolist = first_macro;
        first_macro = NULL;
    }
    riscos_print_save.saved_had_top     = had_top;
    trace_1(TRACE_APP_PD4, "saved had_top %s", trace_boolstring(had_top));
}

static S32
restore_saved_print_state(void)
{
    S32 res = 1;

    in_block            = riscos_print_save.saved_in_block;
    start_block         = riscos_print_save.saved_start_block;
    pagcnt              = riscos_print_save.saved_pagcnt;
    real_pagcnt         = riscos_print_save.saved_real_pagcnt;
    reset_pnm           = riscos_print_save.saved_reset_pnm;
    curpnm              = riscos_print_save.saved_curpnm;
    if(macrofile)
    {
        file_setpos(macrofile, &riscos_print_save.saved_macrofilepos);

        res = duplicate_list(&first_macro, &riscos_print_save.saved_macrolist);
    }
    had_top             = riscos_print_save.saved_had_top;
    trace_1(TRACE_APP_PD4, "restored had_top := %s", trace_boolstring(had_top));

    return(res);
}

static void
finalise_saved_print_state(void)
{
    delete_list(&riscos_print_save.saved_macrolist);
}

/* called to print a bit of a page from RISC OS */

static S32 riscos_res;

static void
application_printpage(void)
{
    trace_0(TRACE_APP_PD4, "application_printpage()");

    riscos_res = restore_saved_print_state();

    if(riscos_res > 0)
        riscos_res = print_page();

    trace_1(TRACE_APP_PD4, "application_printpage got result %d", riscos_res);
}

/******************************************************************************
*
* do the print or spool
* printx does the bulk of the work for spool and print
*
******************************************************************************/

static S32
printx(void)
{
    BOOL firstmacro = TRUE;
    S32 res = 1;
    BOOL triscos_printing, triscos_fonts;

    /* do a print of all files for each macro call */
    while(macrocall()  ||  firstmacro)
    {
        trace_0(TRACE_APP_PD4, "in printx macrocall loop...");

        firstmacro = FALSE;

        /* reset page numbers */
        reset_pnm = TRUE;
        pagcnt = enclns;
        real_pagcnt = 0;

        /* ensure highlight flags are disabled */
        h_waiting = h_on = 0;

        riscos_seqnum = 1;

        /* initialise block, column range, row selection */
        init_print_selection();

        trace_0(TRACE_APP_PD4, "printx done init");

        /* keep printing pages until this macro call complete */
        for(;;)
        {
            trace_0(TRACE_APP_PD4, "printx about to call getfpn");
            getfpn();

            /* does the user want wait between sheets etc. ? */
            if(encpln  &&  sheets_bit)
                do  {
                    int c;
                    char array[LIN_BUFSIZ];

                    prnoff(TRUE);
                    wait_vdu_reset(FALSE);

                    (void) xsnprintf(array, elemof32(array), Page_Zd_wait_between_pages_STR, curpnm);
                    puts(array);

                    clearkeyboardbuffer();

                    c = rdch(TRUE, TRUE);
                    c = toupper(c);

                    if(c == ESCAPE)
                    {
                        bleep();
                        escape_pressed = TRUE;
                        return(create_error(ERR_ESCAPE));              /* end printing please */
                    }
                    else if(strchr(Miss_Page_Chars_STR, c))
                    {
                        print_newline();
                        break;                  /* out of loop */
                    }
                    else if(strchr(All_Pages_Chars_STR, c))
                    {
                        sheets_bit = FALSE;
                    }

                    print_newline();

                    res = prnon();

                    if(res <= 0)
                        return(res ? res : create_error(ERR_GENFAIL));

                    if(riscos_printing)
                    {
                        print_setcolours(FORE, BACK);

                        if(riscos_fonts)
                            print_setfontcolours();
                        else
                            at_print();
                    }
                }
                while(FALSE);

            escape_enable();

            if(riscos_printing  &&  prnbit)
            {
                save_print_state();

                res = riscprint_page(riscos_copies, landscape_option, riscos_scale,
                                     riscos_seqnum++, application_printpage);

                /* no error, or error either from giving page or printing slices */
                res = res ? riscos_res : P_HAD_ERROR;
            }
            else
            {
                /* print page to screen */
                triscos_printing = riscos_printing;
                triscos_fonts    = font_stack(riscos_fonts);

                riscos_printing = FALSE;
                riscos_fonts    = FALSE;
                setpvc();

                res = print_page();

                riscos_printing = triscos_printing;
                riscos_fonts    = font_unstack(triscos_fonts);
                setpvc();
            }

            if(escape_disable())
                res = create_error(ERR_ESCAPE);

            trace_1(TRACE_APP_PD4, "print_page returns %d", res);

            switch(res)
            {
            case P_DONE_PAGE:
                continue;

            case P_FINISHED:
                break;

        /*  case P_HAD_ERROR: */
            default:
                /* it's an error */
                break;
            }

            trace_0(TRACE_APP_PD4, "this macro call finished");
            break;
        }

        /* turn off highlights at the end of each macro invocation */
        off_highlights();

        if(res != P_FINISHED)
            break;
    }   /* each macro */

    return(res);
}

/******************************************************************************
*
*                       low level print routines
*
******************************************************************************/

/******************************************************************************
*
* send character to printer, nothing in the way
*
******************************************************************************/

static void
rawprint(
    _InVal_     U8 ch)
{
    riscos_printing_abort("rawprint");

    if(!ctrlflag && !printer_output_error)
    {
        int res;

#if 0 /* printer_output is UNBUFFERED */
        if(file_putc_fast_ready(printer_output))
        {
            (void) file_putc_fast(ch, printer_output);
            return;
        }
#endif

        res = pd_file_putc(ch, printer_output);

        if(res == EOF)
            printer_output_error = 1;

        return;
    }

    (void) create_error(ERR_ESCAPE);
}

/******************************************************************************
*
* output one character to printer, perhaps
* substituting a printer driver string
*
******************************************************************************/

static BOOL
out_one_ch(
    U8 ch)
{
    PC_LIST lptr;

    riscos_printing_abort("out_one_ch");

    trace_1(TRACE_APP_PD4, "out_one_ch: %x", ch);

    /* try for character substitution */
    lptr = search_list(&highlight_list, (S32) ch);

    if(lptr)
    {
        out_h_string_value_0(lptr);
        return(TRUE);
    }

    rawprint(ch);

    if((ch == CR)  &&  send_linefeeds)
        return(drvout(LF)); /* SKS says ought this to be (rawprint(LF) > 0) ? */

    return(TRUE);
}

static void
out_h_string(
    PC_U8 str,
    S32 thisch)
{
    U8 ch;

    riscos_printing_abort("out_h_string");

    if(!prnbit)
        return;

    while((ch = *str++) != '\0')
    {
        if(ch == ESCAPE)
        {
            ch = *str++;

            if(ch == QUERY)
            {
                out_one_ch(thisch); /* thisch may be expanded */
                continue;
            }

            if(ch == 0xFF)
                ch = 0;
        }

        rawprint(ch);
    }
}

/* a most common call to the above funcion */

static void
out_h_string_value_0(
    PC_LIST lptr)
{
    if(lptr)
        out_h_string(lptr->value, 0);
}

/******************************************************************************
*
* set the driver address in prnvec
*
******************************************************************************/

static void
setpvc(void)
{
    prnvec =
                (riscos_printing)
                    ? &riscos_driver
                    :

                (driver_loaded  &&  !screen_bit /* ok, so it doesn't show remapped chars but what the heck */)
                    ? &loaded_driver
                    : &default_driver;
}

/****************************************************************************** printer driver ******************************/

static S32
riscos_drvon(void)
{
    BOOL ok;

    trace_0(TRACE_APP_PD4, "riscos_drvon()");

    /* only turn on right at start */
    if(out_h_on)
    {
        out_h_on = FALSE;

        ok = riscprint_start();

        if(!ok)
            return(create_error(ERR_GENFAIL));
    }

    /* always resume */
    trace_0(TRACE_APP_PD4, "riscos_drvon: resuming print");
    ok = riscprint_resume();
    return(ok ? ok : create_error(ERR_GENFAIL));
}

static BOOL
riscos_drvoff(
    BOOL ok)
{
    BOOL ok1;

    trace_0(TRACE_APP_PD4, "riscos_drvoff()");

    /* always suspend */
    trace_0(TRACE_APP_PD4, "riscos_drvoff: suspending print");
    ok1 = riscprint_suspend();

    if(ok)
        ok = ok1;

    /*  only end job when fully finished */
    if(out_h_off)
    {
        out_h_off = FALSE;

        riscprint_end(ok);
    }

    return(FALSE);
}

static BOOL
riscos_drvout(
    _InVal_     U8 ch)
{
    BOOL res = TRUE;

    trace_2(TRACE_APP_PD4, "riscos_drvout(%d ('%c'))", ch, ch ? ch : '0');

    switch(ch)
    {
    case CR:
        /* drop baseline one line; doesn't reposition xad */
        riscos_font_yad -= global_font_leading_mp;
        trace_1(TRACE_APP_PD4, "riscos_font_yad = %d (mp) due to CR", riscos_font_yad);
        break;

    case EOS:
        /* switch off highlights (for system font) */
        highlights_on = 0;
        break;

    default:
        res = print_complain(bbc_vdu(ch));
    }

    return(res);
}

/******************************************************************************
*
* printer driver - printer on
*
******************************************************************************/

extern S32
drvon(void)
{
    riscos_printing_abort("drvon");

    trace_0(TRACE_APP_PD4, "drvon()");

    escape_enable();

    if(saved_printer_output)
    {
        /* restore from saved */
        assert(!printer_output); /* out of sync? */
        printer_output = saved_printer_output;
        saved_printer_output = NULL;
    }

    if(out_h_on)
    {
        out_h_on = FALSE;

        /* if the driver has a printer on string, send it out */
        if(driver_loaded)
            out_h_string_value_0(search_list(&highlight_list, P_ON));
    }

    return(escape_disable() ? create_error(ERR_ESCAPE) : 1);
}

/******************************************************************************
*
* printer driver - printer off
*
******************************************************************************/

extern BOOL
drvoff(
    BOOL ok)
{
    riscos_printing_abort("drvoff");

    IGNOREPARM(ok);

    escape_enable();

    if(out_h_off)
    {
        out_h_off = FALSE;

        if(driver_loaded)
            out_h_string_value_0(search_list(&highlight_list, P_OFF));
    }

    assert(!saved_printer_output); /* out of sync? */
    if(printer_output) /* SKS 18.10.91 - don't allow NULL to inadvertently get saved */
    {
        saved_printer_output = printer_output;
        printer_output       = NULL;
    }

    if(d_driver_PT == driver_serial)
    {
        /* flush RS423 buffers */
        fx_x2(21, 1);
    }

    return(!escape_disable());
}

/******************************************************************************
*
* printer driver - output character
*
******************************************************************************/

static BOOL
drvout(
    _InVal_     U8 ch)
{
    S32 mask;
    S32 i;
    PC_LIST lptr;

    riscos_printing_abort("drvout");

    if(!prnbit)
        return(FALSE);

    if(ishighlight(ch))
    {
        mask = (S32) (1 << (ch - FIRST_HIGHLIGHT));

        /* if it's waiting to go on, just turn it off */
        if(h_waiting & mask)
        {
            h_waiting &= ~mask;
            return(FALSE);
        }

        /* if its not on, make it wait */
        if(!(h_on & mask))
        {
            h_waiting |= mask;
            return(FALSE);
        }

        /* it must be on, so turn it off */
        h_on &= ~mask;

        /* if it's a query field don't output anything */
        if(h_query & mask)
            return(FALSE);

        /* if not a query field, use off code if it exists, otherwise on code */
        lptr = search_list(&highlight_list, ((S32) ch) + 256);
        if(!lptr)
            lptr = search_list(&highlight_list, (S32) ch);

        out_h_string_value_0(lptr);

        return(FALSE);
    }

    /* if at end of cell, switch highlights off, maybe */
    if(ch == EOS)
    {
        h_waiting &= ~off_at_cr;

        if(h_on)
        {
            mask = 1;
            i = FIRST_HIGHLIGHT;

            do  {
                /* perhaps don't switch this one off */

                if(!(off_at_cr & mask))
                    continue;

                /* if it's waiting, just stop it waiting */
                if(h_waiting & mask)
                    h_waiting &= ~mask;
                else if((h_on & mask))
                {
                    /* if it's on, turn it off, turn it off, turn it off, ...
                            turn it off again */

                    h_on &= ~mask;

                    /* if it's a query field, don't need to do anything */
                    if(h_query & mask)
                        continue;

                    lptr = search_list(&highlight_list, ((S32) i) + 256);

                    if(!lptr)
                        lptr = search_list(&highlight_list, (S32) i);

                    out_h_string_value_0(lptr);
                }

                mask <<= 1;
            }
            while(++i <= LAST_HIGHLIGHT);
        }

        return(FALSE);
    }

    if(ch == CR)
    {
        if(!prnbit)     /* if outputting to screen */
        {
            print_newline();
            return(TRUE);
        }
    }
    else if(h_waiting)
    {
        /* if something waiting, switch it on */
        mask = 1;
        i = FIRST_HIGHLIGHT;

        do  {
            if(h_waiting & mask)
            {
                /* if itsa query field, don't output anything */
                if(h_query & mask)
                    h_on |= mask;
                else
                {
                    lptr = search_list(&highlight_list, (S32) i);

                    if(lptr)
                    {
                        out_h_string_value_0(lptr);
                        h_on |= mask;
                    }
                }
            }

            mask <<= 1;
        }
        while(++i <= LAST_HIGHLIGHT);

        h_waiting = 0;
    }

    /* output the character.  If any funny highlights expand the strings */
    if(h_query & h_on)
    {
        mask = 1;
        i = FIRST_HIGHLIGHT;

        do  {
            if(h_query & h_on & mask)
            {
                /* if it's a query field, don't output anything */
                lptr = search_list(&highlight_list, (S32) i);
                if(lptr)
                    out_h_string(lptr->value, ch);
            }

            mask <<= 1;
        }
        while(++i <= LAST_HIGHLIGHT);

        return(TRUE);
    }

    return(out_one_ch(ch));
}

/****************************************************************************** default printer driver **************************/

/******************************************************************************
*
* default output character
*
******************************************************************************/

extern BOOL
defout(
    U8 ch)
{
    riscos_printing_abort("defout");

    if((ch == EOS)  ||  ishighlight(ch))
        return(FALSE);

    rawprint(ch);

    if((ch == CR)  &&  !prnbit)
    {
        #if 1 /* SKS 18.10.91 - vdu: does auto LFCR; 20.10.91 but rawvdu: don't */
        print_newline();
        #endif

        if(screen_bit)
        {
            if(++linessofar >= scrnheight())
                (void) pause();
        }
    }

    return(TRUE);
}

/******************************************************************************
*
* send character to printer output routine
* queues spaces for later output/strip
* returns TRUE if printable character
*
******************************************************************************/

extern BOOL
prnout(
    U8 ch)
{
    S32 nspaces;
    BOOL (* out) (_InVal_ U8 ch);

    trace_2(TRACE_APP_PD4, "prnout(%d ('%c'))", ch, ch ? ch : '0');

    if((ch == SPACE)  &&  !microspacing)
    {
        trace_0(TRACE_APP_PD4, "absorbing space");
        ++prnspc;
        return(TRUE);
    }

    out = prnvec->out;
    nspaces = prnspc;

    /* if not end of cell, output spaces cos of printable character */
    if((ch != EOS)  &&  nspaces)
    {
        prnspc = 0;

        /* ignore space queue at end of line */
        if(ch != CR)
            do { (* out) (SPACE); } while(--nspaces > 0);
        else
            trace_1(TRACE_APP_PD4, "discarding %d trailing spaces at eol", nspaces);
    }

    /* output character via driver */
    return((* out) (ch));
}

/******************************************************************************
*
* turn printer on
* printer turned on at beginning
* and at start of each page in sheets
*
******************************************************************************/

static S32
prnon(void)
{
    S32 res;

    prnspc = 0;

    if(!screen_bit  &&  (d_print_QP != 'S'))
        prnbit = TRUE;

    setpvc();

    res = (*prnvec->on) ();      /* switch printer on via driver */

    if(res <= 0)
        prnbit = FALSE;

    return(res);
}

/******************************************************************************
*
* printer turned off at end and maybe end of each page
*
******************************************************************************/

extern void
prnoff(
    BOOL ok)
{
    trace_0(TRACE_APP_PD4, "prnoff()");

    if(!prnbit  &&  !out_h_off)
        return;

    (void) (*prnvec->off) (ok);      /* switch printer off via driver */

    prnbit = out_h_off = FALSE;
}

/******************************************************************************
*
* switch off highlights
*
******************************************************************************/

static void
off_highlights(void)
{
    uchar  mask;
    S32    i;
    PC_LIST lptr;

    if(riscos_printing)
        return;

    mask = 1;
    i = FIRST_HIGHLIGHT;

    while(h_on  &&  (i <= LAST_HIGHLIGHT))
    {
        if(h_on & mask)
        {
            /* turn highlight off with either off string or on string */

            lptr = search_list(&highlight_list, ((S32) i) + 256);

            if(!lptr)
                lptr = search_list(&highlight_list, (S32) i);

            out_h_string_value_0(lptr);
        }

        mask <<= 1;
        ++i;
    }

    h_on = 0;
}

/******************************************************************************
*
* cancel print:
* switch off printer, close spool file etc
*
******************************************************************************/

static void
prncan(
    BOOL ok)
{
    trace_1(TRACE_APP_PD4, "prncan(%s)", trace_boolstring(ok));

    actind_end();

    off_highlights();

    /* delete the macro list */
    delete_list(&first_macro);

    sqobit = FALSE;
    out_h_off = TRUE;

    prnoff(ok);
}

/******************************************************************************
*
* set standard spacing
*
******************************************************************************/

extern void
setssp(void)
{
    smispc = (prnbit  &&  (micbit  ||  riscos_printing))
                        ? d_mspace[1].option : 1;
}

extern BOOL
print_complain(
    os_error * err)
{
    if(err)
    {
        if(riscos_printing)
        {
            riscprint_suspend();

            reperr(create_error(ERR_PRINTER), err->errmess);
        }
        else
            rep_fserr(err->errmess);
    }

    return((BOOL) err);
}

static coord
header_footer_length(
    COL first,
    COL last)
{
    coord maxwidth = 0;
    coord this_colstart = 0;
    coord newwrap;
    COL tcol;

    for(tcol = first; tcol < last; tcol++)
    {
        newwrap = this_colstart + colwrapwidth(tcol);

        maxwidth = MAX(maxwidth, newwrap);

        this_colstart += colwidth(tcol);
    }

    return(MAX(this_colstart, maxwidth));
}

/******************************************************************************
*
* get a field from the driver into a buffer
* if end of file or field too long return false
*
******************************************************************************/

extern int
getfield(
    FILE_HANDLE input,
    uchar *array /*out*/,
    BOOL ignore_lead_spaces)
{
    int ch, newch;
    uchar *ptr;

    if(ignore_lead_spaces)
        while((ch = pd_file_getc(input)) == (int) SPACE)
            ;
    else
        ch = pd_file_getc(input);

    for(ptr = array; ; ch = pd_file_getc(input))
    {
        switch(ch)
        {
        case EOF:
            *ptr = '\0';
            return(EOF);

        case LF:
        case CR:
            {
            /* read next character - if CR,LF or LF,CR throw
             * character away, otherwise put character back in file
            */
            newch = pd_file_getc(input);

            if((newch != EOF)  &&  ((newch + ch) != (LF + CR)))
            {
                if(file_ungetc(newch, input) == EOF)
                {
                    reperr_null(create_error(ERR_CANNOTREAD));
                    *ptr = '\0';
                    return(EOF);
                }
            }
            }
            ch = CR;

            /* deliberate fall through */

        case TAB:
            *ptr = '\0';
            return(ch);

        default:
            *ptr++ = (uchar) ch;

            /* SKS after 4.11 03feb92 - where had this check gone? */
            if(ptr - array >= LIN_BUFSIZ)
            {
                *--ptr = NULLCH;
                return(EOF);
            }
            break;
        }
    }
}

/******************************************************************************
*
* page layout dialog box
*
******************************************************************************/

extern void
PageLayout_fn(void)
{
    if(!dialog_box_start())
        return;

    while(dialog_box(D_POPTIONS))
    {
        update_variables();

        filealtered(TRUE);

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/******************************************************************************
*
* printer configuration dialog box
*
******************************************************************************/

extern void
PrinterConfig_fn(void)
{
    if(!dialog_box_start())
        return;

    status_assert(enumerate_dir_to_list(&ltemplate_or_driver_list, PDRIVERS_SUBDIR_STR, FILETYPE_UNDETERMINED));

    while(dialog_box(D_DRIVER))
    {
        load_driver(&d_driver_PD, FALSE);

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();

    delete_list(&ltemplate_or_driver_list);
}

/******************************************************************************
*
* load printer driver for editing
*
******************************************************************************/

extern void
EditPrinterDriver_fn(void)
{
    if(!dialog_box_start())
        return;

    /* use Load template dialog process 'cos its the same, but initialise for edit driver */

    /* try giving current printer driver as default */
    if(!mystr_set(&d_edit_driver[0].textfield, d_driver_PD))
        return;

    status_assert(enumerate_dir_to_list(&ltemplate_or_driver_list, PDRIVERS_SUBDIR_STR, FILETYPE_UNDETERMINED));

    while(dialog_box(D_EDIT_DRIVER))
    {
        /* Add prefix '<PipeDream$Dir>.PDrivers.' to driver name */
        TCHARZ buffer[BUF_MAX_PATHSTRING];
        LOAD_FILE_OPTIONS load_file_options;
        PCTSTR tname = d_edit_driver[0].textfield;
        S32 res;

        if(str_isblank(tname))
        {
            reperr_null(create_error(ERR_BAD_NAME));
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        if((res = add_path_using_dir(buffer, elemof32(buffer), tname, PDRIVERS_SUBDIR_STR)) <= 0)
        {
            reperr_null(res);
            if(!dialog_box_can_retry())
                break;
            continue;
        }

        zero_struct(load_file_options);
        load_file_options.document_name = buffer;
        load_file_options.filetype_option = AUTO_CHAR; /* therefore must go via loadfile() */

        if(!loadfile(buffer, &load_file_options))
            break;

        xf_file_is_driver = TRUE;

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();

    delete_list(&ltemplate_or_driver_list);
}

/******************************************************************************
*
* has to set micbit, smispc
*
******************************************************************************/

extern void
MicrospacePitch_fn(void)
{
    micbit = FALSE;

    if(!driver_loaded)
    {
        reperr_null(create_error(ERR_NO_DRIVER));
        return;
    }

    if(!search_list(&highlight_list, HMI_P))
    {
        reperr_null(create_error(ERR_NO_MICRO));
        return;
    }

    if(!dialog_box_start())
        return;

    while(dialog_box(D_MSPACE))
    {
        micbit = FALSE;

        if(d_mspace[0].option == 'Y')
        {
            if(!d_mspace[1].option)
            {
                reperr_null(create_error(ERR_BAD_OPTION));
                break;
            }

            micbit = TRUE;
        }

        if(!dialog_box_can_persist())
            break;
    }

    dialog_box_end();
}

/* end of doprint.c */
