/* report.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 2013-2014 Stuart Swales */

/* SKS February 2013 */

#include "common/gflags.h"

#if RISCOS
#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif
#endif

#if RISCOS
#define LOW_MEMORY_LIMIT  0x00008000U /* Don't look in zero page */
#if 1
#define HIGH_MEMORY_LIMIT 0xFFFFFFFCU /* 32-bit RISC OS 5 */
#else
#define HIGH_MEMORY_LIMIT 0xFC000000U
#endif
#endif

static BOOL
g_report_enabled = TRUE;

extern void
report_enable(
    _InVal_     BOOL enable)
{
    g_report_enabled = enable;
}

_Check_return_
extern BOOL
report_enabled(void)
{
    return(g_report_enabled);
}

/******************************************************************************
*
* reportf routine - useful for instrumenting release code
*
******************************************************************************/

extern void __cdecl
reportf(
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        ...)
{
    va_list args;

    if(!g_report_enabled) return;

    va_start(args, format);
    vreportf(format, args);
    va_end(args);
}

/******************************************************************************
*
* vreportf routine
*
******************************************************************************/

static TCHARZ report_buffer[4096];
static U32    report_buffer_offset = 0;

extern void
vreportf(
    _In_z_ _Printf_format_string_ PCTSTR format,
    /**/        va_list args)
{
    PCTSTR tail;
    BOOL wants_continuation;

    if(!g_report_enabled) return;

    if(format[0] == '|')
    {   /* this one is a continuation - doesn't matter if it's already been flushed */
        format++;
    }
    else if(0 != report_buffer_offset)
    {   /* this one is a not a continuation - flush anything pending */
        report_buffer_offset = 0;
        report_output(report_buffer);
    }

    tail = format + tstrlen32(format);
    wants_continuation = (tail != format) && (tail[-1] == '|');

#if WINDOWS
    consume_int(_vsntprintf_s(report_buffer + report_buffer_offset, elemof32(report_buffer) - report_buffer_offset, _TRUNCATE, format, args));
#else/* C99 CRT */
    consume_int(vsnprintf(report_buffer + report_buffer_offset, elemof32(report_buffer) - report_buffer_offset, format, args));
#endif

    if(wants_continuation)
    {
        report_buffer_offset += tstrlen32(report_buffer + report_buffer_offset);
        if(report_buffer[report_buffer_offset - 1] == '|') /* may have been truncated - if so, just output */
        {
            report_buffer[--report_buffer_offset] = NULLCH; /* otherwise retract back over the terminal continuation char */
            return;
        }
    }

    report_buffer_offset = 0;
    report_output(report_buffer);
}

/******************************************************************************
*
* write a string to the reporter module (or file, or whatever else is suitable)
*
******************************************************************************/

#if RISCOS

#ifndef __swis_h
#include "swis.h" /*C:*/
#endif

#define XReport_Text0 (XOS_Bit | /*Report_Text0*/ 0x54C80)

extern int __swi(XReport_Text0) __swi_XReport_Text0(const char * s);

#define XRPCEmu_Debug (XOS_Bit | /*RPCEmu_Debug*/ 0x56AC2)

extern int __swi(XRPCEmu_Debug) __swi_XRPCEmu_Debug(const char * s);

#endif /* RISCOS */

extern void
report_output(
    _In_z_      PCTSTR buffer)
{
    PCTSTR tail;
    BOOL may_need_newline;
    static const TCHARZ newline_buffer[2] = TEXT("\n");

#if RISCOS
    PCTSTR ptr;
    U8 ch;

    static int try_reporter = TRUE;
    static int try_rpcemu_report = TRUE;

    if(!g_report_enabled) return;

    tail = buffer + tstrlen32(buffer);
    may_need_newline = (tail == buffer) || (tail[-1] != '\n');

    if(try_reporter)
    {
        if(__swi_XReport_Text0(buffer) == (int) buffer)
        {   /* Reporter module always sticks a newline on all output */
            return;
        }
        try_reporter = FALSE;
    }

    if(try_rpcemu_report)
    {
        if(__swi_XRPCEmu_Debug(buffer) == (int) buffer)
        {
            if(!may_need_newline)
                return;

            if(__swi_XRPCEmu_Debug(newline_buffer) == (int) newline_buffer)
                return;
        }
        try_rpcemu_report = FALSE;
    }

#if defined(__CC_NORCROFT)
    if(0 == stderr->__file) /* not redirected? */
    {
        trace_disable();
        return;
    }
#endif

    ptr = buffer;

    while((ch = *ptr++) != NULLCH)
        switch(ch)
        {
        case 0x07: /* BEL */
        case 0x0C: /* FF  */
            fputc(ch, stderr);
            break;

        case 0x0A: /* LF  */
        case 0x0D: /* CR  */
            fputc(0x0A, stderr);
            break;

        case 0x7F: /* DEL */
            fputc('|', stderr);
            fputc('?', stderr);
            break;

        default:
            if(ch < 0x20)
            {
                fputc('|', stderr);
                ch = ch + '@';
            }

            fputc(ch, stderr);
            break;
        }

    if(may_need_newline)
        fputc(0x0A, stderr);
#elif WINDOWS
    if(!g_report_enabled) return;

    tail = buffer + tstrlen32(buffer);
    may_need_newline = (tail == buffer) || (tail[-1] != '\n');

    OutputDebugString(buffer);

    if(may_need_newline)
        OutputDebugString(newline_buffer);
#endif /* OS */
}

_Check_return_
_Ret_z_
extern PCTSTR
report_boolstring(
    _InVal_     BOOL t)
{
    return(t ? TEXT("True ") : TEXT("False"));
}

_Check_return_
_Ret_z_
extern PCTSTR
report_procedure_name(
    report_proc proc)
{
    PCTSTR name = TEXT("<<not found>>");
    union _deviant_proc
    {
        report_proc proc;
        uintptr_t value;
        PC_ANY ptr;
    } deviant;

    deviant.proc = proc;

#if RISCOS
    if( ! ( (deviant.value & (uintptr_t) 0x00000003U      ) ||
            (deviant.value < (uintptr_t) LOW_MEMORY_LIMIT ) ||
            (deviant.value > (uintptr_t) HIGH_MEMORY_LIMIT) ) )
    {
        PC_U32 z = (PC_U32) deviant.ptr;
        U32 w = *--z;
        if((w & 0xFFFF0000) == 0xFF000000) /* marker? */
        {
            name = ((PC_U8Z) z) - (w & 0xFFFF);
            return(name);
        }
    }
#endif
    {
    static TCHARZ buffer[16];
#if RISCOS
    consume(int, snprintf(buffer, elemof32(buffer), PTR_XTFMT, deviant.ptr));
#else
    consume(int, _sntprintf_s(buffer, elemof32(buffer), _TRUNCATE, PTR_XTFMT, deviant.ptr));
#endif
    name = buffer;
    } /*block*/

    return(name);
}

_Check_return_
_Ret_z_
extern PCTSTR
report_tstr(
    _In_opt_z_  PCTSTR tstr)
{
    if(IS_PTR_NONE_OR_NULL(PCTSTR, tstr))
        return(TEXT("<<NULL>>"));

#if RISCOS
    if( ((uintptr_t) tstr < (uintptr_t) LOW_MEMORY_LIMIT ) ||
        ((uintptr_t) tstr > (uintptr_t) HIGH_MEMORY_LIMIT) )
#else
    if(IS_BAD_POINTER(tstr))
#endif
    {
        static TCHARZ stringbuffer[16];
#if RISCOS
        consume(int, snprintf(stringbuffer, elemof32(stringbuffer), TEXT("<<") PTR_XTFMT TEXT(">>"), tstr));
#else
        consume(int, _sntprintf_s(stringbuffer, elemof32(stringbuffer), _TRUNCATE, TEXT("<<") PTR_XTFMT TEXT(">>"), tstr));
#endif
        return(stringbuffer);
    }

    return(tstr);
}

_Check_return_
_Ret_z_
extern PCTSTR
report_ustr(
    _In_opt_z_  PC_USTR ustr)
{
    if(IS_PTR_NONE_OR_NULL(PC_USTR, ustr))
        return(TEXT("<<NULL>>"));

#if RISCOS
    if( ((uintptr_t) ustr < (uintptr_t) LOW_MEMORY_LIMIT ) ||
        ((uintptr_t) ustr > (uintptr_t) HIGH_MEMORY_LIMIT) )
#else
    if(IS_BAD_POINTER(ustr))
#endif
    {
        static TCHARZ stringbuffer[16];
#if RISCOS
        consume(int, snprintf(stringbuffer, elemof32(stringbuffer), TEXT("<<") PTR_XTFMT TEXT(">>"), ustr));
#else
        consume(int, _sntprintf_s(stringbuffer, elemof32(stringbuffer), _TRUNCATE, TEXT("<<") PTR_XTFMT TEXT(">>"), ustr));
#endif
        return(stringbuffer);
    }

    return(/*_tstr_from_ustr*/(ustr));
}

_Check_return_
_Ret_z_
extern PCTSTR
report_l1str(
    _In_opt_z_  PC_L1STR l1str)
{
    if(IS_PTR_NONE_OR_NULL(PC_L1STR, l1str))
        return(TEXT("<<NULL>>"));

#if RISCOS
    if( ((uintptr_t) l1str < (uintptr_t) 0x00008000U)  ||
        ((uintptr_t) l1str > (uintptr_t) HIGH_MEMORY_LIMIT))
#else
    if(IS_BAD_POINTER(l1str))
#endif
    {
        static TCHARZ stringbuffer[16];
#if RISCOS
        consume(int, snprintf(stringbuffer, elemof32(stringbuffer), TEXT("<<") PTR_XTFMT TEXT(">>"), l1str));
#else
        consume(int, _sntprintf_s(stringbuffer, elemof32(stringbuffer), _TRUNCATE, TEXT("<<") PTR_XTFMT TEXT(">>"), l1str));
#endif
        return(stringbuffer);
    }

    return(/*_tstr_from_l1str*/(l1str));
}

#if RISCOS

/******************************************************************************
*
* R I S C   O S   s p e c i f i c   r e p o r t i n g   r o u t i n e s
*
******************************************************************************/

_Check_return_
_Ret_z_
extern PCTSTR
report_wimp_event_code(
    _InVal_     int event_code)
{
    static TCHARZ default_eventstring[10];

    switch(event_code)
    {
    case wimp_ENULL:        return(TEXT("wimp_ENULL"));
    case wimp_EREDRAW:      return(TEXT("wimp_EREDRAW"));
    case wimp_EOPEN:        return(TEXT("wimp_EOPEN"));
    case wimp_ECLOSE:       return(TEXT("wimp_ECLOSE"));
    case wimp_EPTRLEAVE:    return(TEXT("wimp_EPTRLEAVE"));
    case wimp_EPTRENTER:    return(TEXT("wimp_EPTRENTER"));
    case wimp_EBUT:         return(TEXT("wimp_EBUT"));
    case wimp_EUSERDRAG:    return(TEXT("wimp_EUSERDRAG"));
    case wimp_EKEY:         return(TEXT("wimp_EKEY"));
    case wimp_EMENU:        return(TEXT("wimp_EMENU"));
    case wimp_ESCROLL:      return(TEXT("wimp_ESCROLL"));
    case wimp_ELOSECARET:   return(TEXT("wimp_ELOSECARET"));
    case wimp_EGAINCARET:   return(TEXT("wimp_EGAINCARET"));

    case wimp_ESEND:        return(TEXT("wimp_ESEND")); /* send message, don't worry if it doesn't arrive */
    case wimp_ESENDWANTACK: return(TEXT("wimp_ESENDWANTACK")); /* send message, return ack if not acknowledged */
    case wimp_EACK:         return(TEXT("wimp_EACK")); /* acknowledge receipt of message */

    default:
        consume(int, snprintf(default_eventstring, elemof32(default_eventstring), U32_XTFMT, (U32) event_code));
        return(default_eventstring);
    }
}

_Check_return_
_Ret_z_
extern PCTSTR
report_wimp_event(
    _InVal_     int event_code,
    _In_        const void * const p_event_data)
{
    const wimp_eventdata * const ed = (const wimp_eventdata * const) p_event_data;
    char tempbuffer[256];

    static TCHARZ messagebuffer[512];

    tstr_xstrkpy(messagebuffer, elemof32(messagebuffer), report_wimp_event_code(event_code));

    switch(event_code)
    {
    case wimp_EOPEN:
        consume(int,
            snprintf(tempbuffer, elemof32(tempbuffer),
                        TEXT(": window ") PTR_XTFMT TEXT("; coords (") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT("); scroll (") S32_TFMT TEXT(", ") S32_TFMT TEXT("); behind &") PTR_XTFMT,
                        (P_ANY) ed->o.w,
                        (S32) ed->o.box.x0, (S32) ed->o.box.y0,
                        (S32) ed->o.box.x1, (S32) ed->o.box.y1,
                        (S32) ed->o.scx,    (S32) ed->o.scy,
                        (P_ANY) ed->o.behind));
        break;

    case wimp_EREDRAW:
    case wimp_ECLOSE:
    case wimp_EPTRLEAVE:
    case wimp_EPTRENTER:
        consume(int,
            snprintf(tempbuffer, elemof32(tempbuffer),
                        TEXT(": window ") PTR_XTFMT,
                        (P_ANY) ed->o.w));
        break;

    case wimp_ESCROLL:
        consume(int,
            snprintf(tempbuffer, elemof32(tempbuffer),
                        TEXT(": window ") PTR_XTFMT TEXT("; coords (") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT(", ") S32_TFMT TEXT("); scroll (") S32_TFMT TEXT(", ") S32_TFMT TEXT("); dir'n (") S32_TFMT TEXT(", ") S32_TFMT TEXT(")"),
                        (P_ANY) ed->scroll.o.w,
                        (S32) ed->scroll.o.box.x0, (S32) ed->scroll.o.box.y0,
                        (S32) ed->scroll.o.box.x1, (S32) ed->scroll.o.box.y1,
                        (S32) ed->scroll.o.scx,    (S32) ed->scroll.o.scy,
                        (S32) ed->scroll.x,        (S32) ed->scroll.y));
        break;

    case wimp_ELOSECARET:
    case wimp_EGAINCARET:
        consume(int,
            snprintf(tempbuffer, elemof32(tempbuffer),
                        TEXT(": window ") PTR_XTFMT TEXT("; icon &%p x ") S32_TFMT TEXT(" y ") S32_TFMT TEXT(" height %8.8X index ") S32_TFMT,
                        (P_ANY) ed->c.w,
                        (P_ANY) ed->c.i,
                        (S32) ed->c.x,      (S32) ed->c.y,
                        (S32) ed->c.height, (S32) ed->c.index));

        break;

    case wimp_ESEND:
    case wimp_ESENDWANTACK:
    case wimp_EACK:
        consume(int,
            snprintf(tempbuffer, elemof32(tempbuffer),
                        TEXT(": %s"), report_wimp_message(&ed->msg, FALSE)));
        break;

    case wimp_ENULL:
    case wimp_EBUT:
    case wimp_EKEY:
    case wimp_EMENU:

    default:
        *tempbuffer = NULLCH;
        break;
    }

    if(*tempbuffer)
        tstr_xstrkat(messagebuffer, elemof32(messagebuffer), tempbuffer);

    return(messagebuffer);
}

_Check_return_
_Ret_z_
extern PCTSTR
report_wimp_message_action(
    _InVal_     int message_action)
{
    static TCHARZ default_actionstring[10];

    switch(message_action)
    {
    case wimp_MCLOSEDOWN:       return(TEXT("wimp_MCLOSEDOWN"));

    case wimp_MDATASAVE:        return(TEXT("wimp_MDATASAVE")); /* request to identify directory */
    case wimp_MDATASAVEOK:      return(TEXT("wimp_MDATASAVEOK")); /* reply to wimp_MDATASAVE */
    case wimp_MDATALOAD:        return(TEXT("wimp_MDATALOAD")); /* request to load/insert dragged icon */
    case wimp_MDATALOADOK:      return(TEXT("wimp_MDATALOADOK")); /* reply that file has been loaded */
    case wimp_MDATAOPEN:        return(TEXT("wimp_MDATAOPEN")); /* warning that an object is to be opened */
    case wimp_MRAMFETCH:        return(TEXT("wimp_MRAMFETCH")); /* transfer data to buffer in my workspace */
    case wimp_MRAMTRANSMIT:     return(TEXT("wimp_MRAMTRANSMIT")); /* I have transferred some data to a buffer in your workspace */

    case wimp_MPREQUIT:         return(TEXT("wimp_MPREQUIT"));
    case wimp_PALETTECHANGE:    return(TEXT("wimp_MPALETTECHANGE"));

    case wimp_FilerOpenDir:     return(TEXT("wimp_MFilerOpenDir"));
    case wimp_FilerCloseDir:    return(TEXT("wimp_MFilerCloseDir"));

    case wimp_MHELPREQUEST:     return(TEXT("wimp_MHELPREQUEST")); /* interactive help request */
    case wimp_MHELPREPLY:       return(TEXT("wimp_MHELPREPLY")); /* interactive help message */

    case wimp_Notify:           return(TEXT("wimp_MNOTIFY"));

    case wimp_MMENUWARN:        return(TEXT("wimp_MMENUWARN")); /* menu warning */
    case wimp_MMODECHANGE:      return(TEXT("wimp_MMODECHANGE"));
    case wimp_MINITTASK:        return(TEXT("wimp_MTASKINIT"));
    case wimp_MCLOSETASK:       return(TEXT("wimp_MCLOSETASK"));
    case wimp_MSLOTCHANGE:      return(TEXT("wimp_MSLOTCHANGE")); /* Slot size has altered */
    case wimp_MSETSLOT:         return(TEXT("wimp_MSETSLOT"));
    case wimp_MTASKNAMERQ:      return(TEXT("wimp_MTASKNAMERQ"));
    case wimp_MTASKNAMEIS:      return(TEXT("wimp_MTASKNAMEIS"));

    /* Messages for dialogue with printer applications */
    case wimp_MPrintFile:       return(TEXT("wimp_MPrintFile"));
    case wimp_MWillPrint:       return(TEXT("wimp_MWillPrint"));
    case wimp_MPrintTypeOdd:    return(TEXT("wimp_MPrintTypeOdd"));
    case wimp_MPrintTypeKnown:  return(TEXT("wimp_MPrintTypeKnown"));
    case wimp_MPrinterChange:   return(TEXT("wimp_MPrinterChange"));

    default:
        consume(int, snprintf(default_actionstring, elemof32(default_actionstring), U32_XTFMT, (U32) message_action));
        return(default_actionstring);
    }
}

_Check_return_
_Ret_z_
extern PCTSTR
report_wimp_message(
    _In_        const void * const p_wimp_message,
    _InVal_     BOOL sending)
{
    const wimp_msgstr * const m = (const wimp_msgstr * const) p_wimp_message;
    PCTSTR format =
        sending
            ? TEXT("type %s, size ") S32_TFMT TEXT(", your_ref ") S32_TFMT TEXT(" ")
            : TEXT("type %s, size ") S32_TFMT TEXT(", your_ref ") S32_TFMT TEXT(" from task &%8.8X, my(his)_ref ") S32_TFMT TEXT(" ");
    TCHARZ tempbuffer[256];

    static TCHARZ messagebuffer[512];

    consume(int,
        snprintf(messagebuffer, elemof32(messagebuffer), format,
                 report_wimp_message_action(m->hdr.action),
                 m->hdr.size, m->hdr.your_ref,
                 m->hdr.task, m->hdr.my_ref));

    switch(m->hdr.action)
    {
    case wimp_MINITTASK:
        consume(int,
            snprintf(tempbuffer, elemof32(tempbuffer), "CAO &%p AplSize &%8.8X TaskName \"%s\"",
                     m->data.taskinit.CAO,
                     m->data.taskinit.size,
                     m->data.taskinit.taskname));
        break;

    case wimp_MCLOSEDOWN:

    case wimp_MDATASAVE: /* request to identify directory */
    case wimp_MDATASAVEOK: /* reply to wimp_MDATASAVE */
    case wimp_MDATALOAD: /* request to load/insert dragged icon */
    case wimp_MDATALOADOK: /* reply that file has been loaded */
    case wimp_MDATAOPEN: /* warning that an object is to be opened */
    case wimp_MRAMFETCH: /* transfer data to buffer in my workspace */
    case wimp_MRAMTRANSMIT: /* I have transferred some data to a buffer in your workspace */

    case wimp_MPREQUIT:
    case wimp_PALETTECHANGE:

    case wimp_FilerOpenDir:
    case wimp_FilerCloseDir:

    case wimp_MHELPREQUEST:
    case wimp_MHELPREPLY:

    case wimp_Notify:

    case wimp_MMENUWARN:
    case wimp_MMODECHANGE:
    case wimp_MCLOSETASK:
    case wimp_MSLOTCHANGE:
    case wimp_MSETSLOT:
    case wimp_MTASKNAMERQ:
    case wimp_MTASKNAMEIS:

    /* Messages for dialogue with printer applications */

    case wimp_MPrintFile:
    case wimp_MWillPrint:
    case wimp_MPrintTypeOdd:
    case wimp_MPrintTypeKnown:
    case wimp_MPrinterChange:

    default:
        *tempbuffer = NULLCH;
        break;
    }

    if(*tempbuffer)
        tstr_xstrkat(messagebuffer, elemof32(messagebuffer), tempbuffer);

    return(messagebuffer);
}

#endif /* RISCOS */

/* end of report.c */
