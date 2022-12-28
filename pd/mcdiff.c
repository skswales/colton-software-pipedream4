/* mcdiff.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1987-1998 Colton Software Limited
 * Copyright (C) 1998-2015 R W Colton */

/* Machine dependent routines for screen & keyboard */

/* RJM Aug 87 */

#include "common/gflags.h"

#if RISCOS
#include <locale.h>
#include "cs-kernel.h"
#include "swis.h" /*C:*/

#ifndef __cs_bbcx_h
#include "cs-bbcx.h"
#endif

#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif

#ifndef __cs_wimptx_h
#include "cs-wimptx.h"
#endif

#endif

#include "datafmt.h"

#include "riscos_x.h"
#include "riscdraw.h"

/******************************************************************************
*
*  write character with highlight
*
******************************************************************************/

/* We know how to redefine SPACE ! */
#define victimchar SPACE

extern void
wrch_h(
    char ch)
{
    char array[10];
    char *ptr;
    char *toprow    = array + 1;
    char *midway    = array + 4;
    char *baseline  = array + 7;
    char *from;

    array[0] = ch;
    (void) _kernel_osword(10, (int *) array); /* read char definition */

    if(highlights_on & N_ITALIC)
    {
        ptr = toprow;
        do { *ptr = *ptr >> 1; } while(++ptr <= midway);    /* stagger top half */
    }

    if(highlights_on & N_BOLD)
    {
        ptr = toprow;
        do { *ptr = *ptr | (*ptr << 1); } while(++ptr <= baseline + 1); /* embolden */
    }

    if(highlights_on & N_SUPERSCRIPT)
    {
        from = toprow + 2;
        ptr = toprow + 1;
        do { *ptr++ = *from; from += 2; } while(ptr <= midway);

        do { *ptr++ = CH_NULL; } while(ptr <= baseline + 1);
    }
    else if(highlights_on & N_SUBSCRIPT)
    {
        from = baseline;
        ptr = baseline + 1;
        do { *ptr-- = *from; from -=2; } while(ptr > midway);

        do { *ptr-- = CH_NULL; } while(ptr >= toprow);
    }

#if 1
    static U8 redefine_space_output_restore_space[10 + 1 + 10] =
    {
        bbc_MultiPurpose, SPACE, 0,0,0,0,0,0,0,0,
        SPACE,
        bbc_MultiPurpose, SPACE, 0,0,0,0,0,0,0,0
    };
    P_U8 o_ptr = redefine_space_output_restore_space + 2;

    ptr = toprow;
    do { *o_ptr++ = *ptr++; } while(ptr <= baseline);

    if(highlights_on & N_UNDERLINE)
        *ptr ^= 0xFF;                                       /* EOR underline in */

    *o_ptr = *ptr;                                          /* complete char redefinition */

    /* redefine, output, and restore all in one go */
    print_complain(os_writeN(redefine_space_output_restore_space, elemof32(redefine_space_output_restore_space)));
#else
    print_complain(bbc_vdu(bbc_MultiPurpose));              /* start to redefine char */
    print_complain(bbc_vdu(victimchar));

    ptr = toprow;
    do { print_complain(bbc_vdu(*ptr++)); } while(ptr <= baseline);

    if(highlights_on & N_UNDERLINE)
        *ptr ^= 0xFF;                                       /* EOR underline in */

    print_complain(bbc_vdu(*ptr));                          /* complete char redefinition */

    /* print character now that it's defined */
    print_complain(bbc_vdu(victimchar));

    /* redefine SPACE after abusing it so */
    static const U8 restore_space[10] = { bbc_MultiPurpose, SPACE, 0,0,0,0,0,0,0,0 };
    print_complain(os_writeN(restore_space, elemof32(restore_space)));
    //print_complain(bbc_vduq(bbc_MultiPurpose, SPACE, 0,0,0,0,0,0,0,0));
#endif
}

/* --------------------------- RISCOS only ------------------------------- */

#define MOUSE_BUFFER_NUMBER     9
#define MOUSE_BUFFER_INKEY_ID   246
#define MOUSE_BUFFER_EVENT_SIZE 9

extern void
clearmousebuffer(void)
{
    (void) _kernel_osbyte(21, MOUSE_BUFFER_NUMBER, 0);
}

/******************************************************************************
*
* set background colour to specified colour
* for things that are never inverted and
* always printing using current foreground
*
******************************************************************************/

extern void
set_bg_colour_from_option(
    _InVal_     COLOURS_OPTION_INDEX bg_colours_option_index)
{
    riscos_set_bg_colour_from_wimp_colour_value(wimp_colour_value_from_option(bg_colours_option_index));
}

/******************************************************************************
*
* set foreground colour to specified colour
* for things that are never inverted and
* always printing over current background
*
******************************************************************************/

extern void
set_fg_colour_from_option(
    _InVal_     COLOURS_OPTION_INDEX fg_colours_option_index)
{
    riscos_set_fg_colour_from_wimp_colour_value(wimp_colour_value_from_option(fg_colours_option_index));
}

/******************************************************************************
*
* set fg and bg colour to those specified
* or the other way round if currently_inverted
*
******************************************************************************/

extern void
setcolours(
    _In_          COLOURS_OPTION_INDEX fg_colours_option_index,
    _In_          COLOURS_OPTION_INDEX bg_colours_option_index)
{
    if(currently_inverted)
    {
        if((fg_colours_option_index == COI_FORE)  &&  (bg_colours_option_index == COI_BACK))
        {
            fg_colours_option_index = COI_BACK;
            bg_colours_option_index = COI_FORE;
        }
        else if((fg_colours_option_index == COI_BACK)  &&  (bg_colours_option_index == COI_FORE))
        {
            fg_colours_option_index = COI_FORE;
            bg_colours_option_index = COI_BACK;
        }
    }

    set_bg_colour_from_option(bg_colours_option_index);
    set_fg_colour_from_option(fg_colours_option_index);
}

/* ------------------- Ones that are basically similar ------------------- */

extern void
ack_esc(void)
{
    trace_0(TRACE_APP_PD4, "ack_esc()");

    if(ctrlflag)
    {
        (void) _kernel_osbyte(126, 0, 0);

        ctrlflag = 0;
    }
}

extern void
bleep(void)
{
    (void) bbc_vdu(bbc_Bell);
}

extern void
clearkeyboardbuffer(void)
{
    (void) _kernel_osbyte(15, 1, 0);

    clearmousebuffer();
}

#if 0

/* returns TRUE if key is depressed */

static inline BOOL
depressed(
    S32 key)
{
    return((0xFF & _kernel_osbyte(129, key, 0xFF)) == (S32) 0xFF);
}

extern BOOL
host_ctrl_pressed(void)
{
    return(depressed(CTRL));
}

extern BOOL
host_shift_pressed(void)
{
    return(depressed(SHIFT));
}

#endif

extern BOOL /*ctrl_pressed*/
host_keyboard_status(
    _OutRef_    P_BOOL p_shift_pressed)
{
    const int r = (0xFF & _kernel_osbyte(202, 0, 0xFF));
    *p_shift_pressed = (0 != (r & (1 << 3)));
    return(0 != (r & (1 << 6))); /*ctrl_pressed*/
}

/******************************************************************************
*
*  scroll from first_line to bos down one line
*
******************************************************************************/

extern void
down_scroll(
    coord first_line)
{
    trace_1(TRACE_APP_PD4, "down_scroll(%d)", first_line);

    scroll_textarea(-1 /*lh slop*/, paghyt, pagwid_plus1, first_line, 1);
}

/******************************************************************************
*
* On RISC OS, disable ESCAPE condition generation.
* On all systems, report an error if such a condition was present
* and clear that condition. Returns the old condition.
*
******************************************************************************/

static int escape_level = 0;

extern S32
escape_disable_nowinge(void)
{
    BOOL was_ctrlflag = FALSE;

    if(0 == escape_level)
        return(0);

    if(--escape_level == 0)
    {
        (void) _kernel_osbyte(229, 1, 0); /* Ensure ESCAPE disabled */

        was_ctrlflag = ctrlflag;

        if(was_ctrlflag)
            ack_esc();          /* clears ctrlflag */
    }

    return(was_ctrlflag ? ERR_ESCAPE : 0);
}

extern BOOL
escape_disable(void)
{
    S32 res = escape_disable_nowinge();

    if(res < 0)
        reperr_null(res);

    return(res < 0);
}

extern void
escape_enable(void)
{
    if(escape_level++ == 0)
        (void) _kernel_osbyte(229, 0, 0); /* Ensure ESCAPE enabled */
}

/******************************************************************************
*
* invert the text fg and bg colours
* The current state is maintained by currently_inverted
*
******************************************************************************/

extern void
invert(void)
{
    currently_inverted = !currently_inverted;

    riscos_invert();
}

extern BOOL
keyinbuffer(void)
{
    _kernel_swi_regs rs;
    int carry;

    (void) _kernel_swi_c(OS_ReadEscapeState, &rs, &rs, &carry);
    /* When we do get() we'll get ESC */

    if(carry != 0)
    {
        trace_0(TRACE_APP_PD4, "keyinbuffer returns TRUE due to ESCAPE set");
        return(TRUE);
    }

    rs.r[0] = 152;      /* Examine keyboard buffer */
    rs.r[1] = 0;
    rs.r[2] = 0;
    (void) _kernel_swi_c(OS_Byte, &rs, &rs, &carry);

#if TRACE_ALLOWED
    if(!carry)
    {
        trace_0(TRACE_APP_PD4, "keyinbuffer returns TRUE");
    }
    else
    {
        /*trace_0(TRACE_APP_PD4, "keyinbuffer returns FALSE");*/
    }
#endif

    return(carry == 0);
}

/******************************************************************************
*
*  Initialise machine and library state for PipeDream
*
******************************************************************************/

static BOOL mc__initialised = FALSE;

extern void
init_mc(void)
{
/* screen variables are determined by the opened window */

    /* set locale appropriately */
    if(NULL != setlocale(LC_ALL, DefaultLocale_STR))
        reportf("setlocale(%s) => %s", DefaultLocale_STR, setlocale(LC_ALL, NULL));
    else
        consume_ptr(setlocale(LC_ALL, "C"));

    status_consume(add_to_list(&first_key,  KMAP_FUNC_HOME,       "\031" "ctc" "\x0D"));  /* Top Of Column */
    status_consume(add_to_list(&first_key, KMAP_FUNC_CHOME,       "\031" "ctc" "\x0D"));  /* Top Of Column */

    status_consume(add_to_list(&first_key,  KMAP_FUNC_END,        "\031" "cbc" "\x0D"));  /* Bottom Of Column */
    status_consume(add_to_list(&first_key, KMAP_FUNC_CEND,        "\031" "cbc" "\x0D"));  /* Bottom Of Column */

  /*status_consume(add_to_list(&first_key,  KMAP_FUNC_CDELETE,    "\031" "crb" "\x0D"));*//* Delete Character Left */
  /*status_consume(add_to_list(&first_key, KMAP_FUNC_CSDELETE,    "\031" "g"   "\x0D"));*//* Delete Character Right */

    status_consume(add_to_list(&first_key,   KMAP_FUNC_BACKSPACE, "\031" "crb" "\x0D"));  /* Delete Character Left */
    status_consume(add_to_list(&first_key,  KMAP_FUNC_SBACKSPACE, "\031" "g"   "\x0D"));  /* Delete Character Right */
  /*status_consume(add_to_list(&first_key,  KMAP_FUNC_CBACKSPACE, "\031" "crb" "\x0D"));*//* Delete Character Left */
  /*status_consume(add_to_list(&first_key, KMAP_FUNC_CSBACKSPACE, "\031" "g"   "\x0D"));*//* Delete Character Right */

    mc__initialised = TRUE;
}

extern void
reset_mc(void)
{
    if(mc__initialised)
    {
        mc__initialised = FALSE;

        close_user_dictionaries(TRUE);
    }
}

/******************************************************************************
*
* rdch macroed on RISCOS to rdch_riscos
*
* SKS notes that we can't use signal(SIGINT) as we rely on being able to
* read the two bytes of a function key sequence and one or both may be
* purged by the C runtime library after keyinbuffer() or the first get()
*
******************************************************************************/

extern S32
rdch_riscos(void)
{
    S32 c;

    c = bbc_get() & 0xFF;   /* C from OS_ReadC returned in 0x100 */

    if(c)
        return(c);

    (void) bbc_get();

    return(CR); /* approximate - only needed for printing wait between pages */
}

/******************************************************************************
*
* offset of bottom text line on screen
* NB at(0, scrnheight) should put cursor at bottom of screen
*
******************************************************************************/

extern coord
scrnheight(void)
{
    return((coord) bbc_vduvar(bbc_ScrBCol));
}

/******************************************************************************
*
* output string in current highlight state
*
* returns number of characters printed
*
******************************************************************************/

extern coord
stringout(
    const char *str)
{
    coord len;

    char ch;
    len = strlen(str);
    #if TRUE
    while((ch = *str++) != CH_NULL)
        sndchr(ch);
    #else
    if(highlights_on)
        while((ch = *str++) != CH_NULL)
            wrch_h(ch);
    else
        print_complain(os_write0, str));
    #endif

    return(len);
}

/******************************************************************************
*
* scroll from first_line to bos up one line
*
******************************************************************************/

extern void
up_scroll(
    coord first_line)
{
    trace_1(TRACE_APP_PD4, "up_scroll(%d)", first_line);

    scroll_textarea(-1 /*lh slop*/, paghyt, pagwid_plus1, first_line, -1);
}

/******************************************************************************
*
* Draw i of one char to screen.
*
******************************************************************************/

extern void
wrchrep(
    uchar ch,
    coord i)
{
    if(sqobit)
    {
        while(i-- > 0)
            prnout(ch);
        return;
    }

    while(i-- > 0)
        print_complain(bbc_vdu(ch));
}

/* end of mcdiff.c */
