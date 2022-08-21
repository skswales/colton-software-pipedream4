; cs-pollfpm.s

; This Source Code Form is subject to the terms of the Mozilla Public
; License, v. 2.0. If a copy of the MPL was not distributed with this
; file, You can obtain one at http://mozilla.org/MPL/2.0/.

; Copyright (C) 1989-1998 Colton Software Limited
; Copyright (C) 1998-2014 R W Colton

; SKS for Colton Software optimised use now we can rely on /fpe3 LFM/SFM (wimp_poll_fpm)

; Automagically does hourglass on/off on entry/exit

        GET     as_flags_h

        GET     as_regs_h
        GET     as_macro_h

XOS_Bit                 * 1 :SHL: 17

XOS_ReadMonotonicTime   * XOS_Bit :OR: &000042

XWimp_Poll              * XOS_Bit :OR: &0400C7
XWimp_PollIdle          * XOS_Bit :OR: &0400E1

XHourglass_Off          * XOS_Bit :OR: &0406c1
XHourglass_Start        * XOS_Bit :OR: &0406c3

 [ PROFILING
; Keep consistent with cmodules/riscos/profile.h
XProfiler_On            * XOS_Bit :OR: &88001
XProfiler_Off           * XOS_Bit :OR: &88002
 ]

                GBLL    POLL_HOURGLASS      ; Whether to do hourglass off/on over polling
POLL_HOURGLASS  SETL    {TRUE}

POLL_IDLE_TIME      * 2 ; cs delay till next null event

Wimp_Poll_NullMask  * 1 ; (1 << Wimp_ENull)

Wimp_ERedrawWindow  * 1
NORMAL_DELAY_PERIOD * 100; /* more suitable for normal apps than 33cs default */
REDRAW_DELAY_PERIOD * 200; /* default (cs) */

        AREA    |C$$code|,CODE,READONLY

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
; extern os_error *
; wimp_Poll_SFM(
; r0 a1     wimp_emask mask,
; r1 a2     wimp_eventstr * result /*out*/);

        BeginExternal wimp_Poll_SFM

        FunctionEntry "v1,v2", "MakeFrame"

        MOV     v1, r0                      ; save mask
        MOV     v2, r1                      ; save eventstr^

 [ PROFILING
        SWI     XProfiler_Off
 ]
 [ POLL_HOURGLASS
        SWI     XHourglass_Off
 ]
        SUB     sp, sp, #4*(4*3)
        RFS     ip
        SFM     f4, 4, [sp, #0]
        STMFD   sp!, {ip}

        TST     v1, #Wimp_Poll_NullMask
        SWIEQ   XOS_ReadMonotonicTime
        ADDEQ   r2, r0, #POLL_IDLE_TIME     ; return nulls only every so often
        BEQ     wimp_pollidle_fpm_common

        MOV     r0, v1                      ; restore mask
        ADD     r1, v2, #4                  ; &eventstr->data
        SWI     XWimp_Poll

 [ PROFILING
        B       wimp_poll_fpm_common_exit

        BeginInternal wimp_poll_fpm_common_exit
 |
wimp_poll_fpm_common_exit
 ]

; comes here from below too

        STR     r0, [v2, #0]                ; eventstr->e = rc;
        MOVVS   v1, r0                      ; save error^
        MOVVC   v1, #0                      ; save zero -> no error

        LDMFD   sp!, {ip}
        WFS     ip
        LFM     f4, 4, [sp, #0]
        ADD     sp, sp, #4*12

 [ POLL_HOURGLASS
        TEQ     r0, #Wimp_ERedrawWindow
        MOVEQ   r0, #REDRAW_DELAY_PERIOD
        MOVNE   r0, #NORMAL_DELAY_PERIOD
        SWI     XHourglass_Start
 ]
 [ PROFILING
        SWI     XProfiler_On
 ]

        MOV     r0, v1                      ; restore error^ or zero

        FunctionReturn "v1,v2", "fpbased"

; .............................................................................
; extern os_error *
; wimp_pollidle_fpm(
; r0 a1     wimp_emask mask,
; r1 a2     wimp_eventstr * result /*out*/,
; r2 a3     int earliest);

        BeginExternal wimp_pollidle_fpm

        FunctionEntry "v1,v2", "MakeFrame"

        MOV     v1, r0
        MOV     v2, r1

 [ PROFILING
        SWI     XProfiler_Off
 ]
 [ POLL_HOURGLASS
        SWI     XHourglass_Off
 ]

        SUB     sp, sp, #4*12
        RFS     ip
        SFM     f4, 4, [sp, #0]
        STMFD   sp!, {ip}

wimp_pollidle_fpm_common ; branched here from null event case of above

        MOV     r0, v1                      ; restore mask
        ADD     r1, v2, #4                  ; &eventstr->data
        SWI     XWimp_PollIdle

        B       wimp_poll_fpm_common_exit

; +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 [ PROFILING
        BeginInternal wimp_poll_fpm_ends_here
 ]

        END     ; of s.cs-pollfpm
