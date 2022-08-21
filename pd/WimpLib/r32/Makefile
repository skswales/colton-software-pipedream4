# WimpLib.release (32-bit)

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# Copyright (C) 1989-1999 Colton Software
# Copyright (C) 1998-2015 R W Colton

#URD=$.pipedream4.trunk

WimpLib=^

PD=$(WimpLib).^

CMODULES=$(PD).cmodules

# Use current set of 32-bit tools on Run$Path and libraries on C$Path

AS=ObjAsm
# --arm to allow assembly of UAL syntax code
# -cpu 3M to allow assembly of S/UMULL
ASflags_BASE=-I$(PD).common.Hdr --arm --arm_only -cpu 3M --throwback
ASflags=$(ASflags_BASE)

# NB PipeDream uses a global register for its sb variable, so avoid using that in this library code!

CC=cc
CCflags_BASE=-c -I$(WimpLib) -I$(PD) -DSB_GLOBAL_REGISTER -fafh -Wp -throwback
# -arch 2 -cpu 3 gives code I prefer to look at e.g. STMs, multiple exit LDMs
CCflags_BASE=-c -I$(WimpLib) -I$(PD) -DSB_GLOBAL_REGISTER -fafh -Wp -arch 2 -cpu 3 -throwback
# -arch 3M to allow use of S/UMULL with -Otime
#CCflags_BASE=-c -I$(WimpLib) -I$(PD) -DSB_GLOBAL_REGISTER -fafh -Wp -arch 3M -cpu 7 -DNORCROFT_ARCH_M -throwback
CCflags=$(CCflags_BASE) -Otime
#CCflags=$(CCflags_BASE) -g

CCflags_CSX=$(CCflags) -DCOMPILING_WIMPLIB_EXTENSIONS

LibFile=libfile
LibFileflags=-o -c $@

WimpLib: o.WimpLib

# Colton Software extras, replacement and wrappers of patched files

o.cs-bbcx:     $(WimpLib).c.bbc $(WimpLib).h.bbc
o.cs-bbcx:     $(WimpLib).h.cs-bbcx
o.cs-bbcx:     $(WimpLib).c.cs-bbcx
        $(CC) -o $@ $(CCflags_CSX) $(WimpLib).c.cs-bbcx

o.cs-event:    $(WimpLib).c.event $(WimpLib).h.event
o.cs-event:    $(WimpLib).c.cs-event
        $(CC) -o $@ $(CCflags_CSX) $(WimpLib).c.cs-event

o.cs-flex:     $(WimpLib).c.flex
o.cs-flex:     $(WimpLib).h.cs-flex
o.cs-flex:     $(WimpLib).c.cs-flex
        $(CC) -o $@ $(CCflags_CSX) $(WimpLib).c.cs-flex

o.cs-resspr:   $(WimpLib).h.cs-resspr
o.cs-resspr:   $(WimpLib).c.cs-resspr
        $(CC) -o $@ $(CCflags_CSX) $(WimpLib).c.cs-resspr

o.cs-wimptx:   $(WimpLib).c.wimpt $(WimpLib).h.wimpt
o.cs-wimptx:   $(WimpLib).h.cs-wimptx
o.cs-wimptx:   $(WimpLib).c.cs-wimptx
        $(CC) -o $@ $(CCflags_CSX) $(WimpLib).c.cs-wimptx

o.cs-winf:     $(WimpLib).h.cs-winf
o.cs-winf:     $(WimpLib).c.cs-winf
        $(CC) -o $@ $(CCflags_CSX) $(WimpLib).c.cs-winf

o.cs-winx:     $(WimpLib).c.win $(WimpLib).h.win
o.cs-winx:     $(WimpLib).h.cs-winx
o.cs-winx:     $(WimpLib).c.cs-winx
        $(CC) -o $@ $(CCflags_CSX) $(WimpLib).c.cs-winx

o.cs-riscasm:  $(WimpLib).s.cs-riscasm
        $(AS) -to $@ $(ASflags) -from $(WimpLib).s.cs-riscasm

o.cs-poll:     $(WimpLib).s.cs-poll
        $(AS) -to $@ $(ASflags) -from $(WimpLib).s.cs-poll

# Colton Software replacements

o.cs-msgs:     $(WimpLib).h.msgs
o.cs-msgs:     $(WimpLib).c.cs-msgs
        $(CC) -o $@ $(CCflags_CSX) $(WimpLib).c.cs-msgs

o.cs-templat:  $(WimpLib).h.template
o.cs-templat:  $(WimpLib).h.cs-template
o.cs-templat:  $(WimpLib).c.cs-templat
        $(CC) -o $@ $(CCflags_CSX) $(WimpLib).c.cs-templat

COLTON_XTRAS=\
o.cs-bbcx \
o.cs-event \
o.cs-flex \
o.cs-msgs \
o.cs-resspr \
o.cs-templat \
o.cs-wimptx \
o.cs-winf \
o.cs-winx \
o.cs-poll \
o.cs-riscasm

# source files with deltas (patched from RISC_OSLib source, without extra wrapping)

o.dbox:        $(WimpLib).h.cs-dbox
o.dbox:        $(WimpLib).c.dbox
        $(CC) -o $@ $(CCflags_CSX) $(WimpLib).c.dbox

o.fontlist:    $(WimpLib).c.fontlist
        $(CC) -o $@ $(CCflags) $(WimpLib).c.fontlist

o.fontselect:  $(WimpLib).c.fontselect
        $(CC) -o $@ $(CCflags_CSX) $(WimpLib).c.fontselect

o.menu:        $(WimpLib).h.menu
o.menu:        $(WimpLib).c.menu
        $(CC) -o $@ $(CCflags) $(WimpLib).c.menu

o.res:         $(WimpLib).h.res
o.res:         $(WimpLib).c.res
        $(CC) -o $@ $(CCflags) $(WimpLib).c.res

o.wimp:        $(WimpLib).h.wimp
o.wimp:        $(WimpLib).c.wimp
        $(CC) -o $@ $(CCflags) $(WimpLib).c.wimp

o.xferrecv:    $(WimpLib).h.xferrecv $(WimpLib).h.cs-xferrecv
o.xferrecv:    $(WimpLib).c.xferrecv
        $(CC) -o $@ $(CCflags) $(WimpLib).c.xferrecv

o.xfersend:    $(WimpLib).h.xfersend $(WimpLib).h.cs-xfersend
o.xfersend:    $(WimpLib).c.xfersend
        $(CC) -o $@ $(CCflags) $(WimpLib).c.xfersend

o.swi:         $(WimpLib).s.swi
        $(AS) -to $@ $(ASflags) -from $(WimpLib).s.swi

PATCHED=\
o.dbox \
o.fontlist \
o.fontselect \
o.menu \
o.res \
o.wimp \
o.xferrecv \
o.xfersend \
o.swi

# source files with no deltas (but wrapped with nd-xxx.c for context)

o.drawcheck:   $(WimpLib).c.drawcheck $(WimpLib).c.nd-drawcheck
        $(CC) -o $@ $(CCflags) $(WimpLib).c.nd-drawcheck

o.drawfdiag:   $(WimpLib).c.drawfdiag $(WimpLib).c.nd-drawfdiag
        $(CC) -o $@ $(CCflags) $(WimpLib).c.nd-drawfdiag

o.drawfobj:    $(WimpLib).c.drawfobj $(WimpLib).c.nd-drawfobj
        $(CC) -o $@ $(CCflags) $(WimpLib).c.nd-drawfobj

o.drawmod:     $(WimpLib).c.drawmod $(WimpLib).c.nd-drawmod
        $(CC) -o $@ $(CCflags) $(WimpLib).c.nd-drawmod

o.font:        $(WimpLib).c.font $(WimpLib).c.nd-font
        $(CC) -o $@ $(CCflags) $(WimpLib).c.nd-font

o.fileicon:    $(WimpLib).c.fileicon $(WimpLib).c.nd-fileicon
        $(CC) -o $@ $(CCflags) $(WimpLib).c.nd-fileicon

o.os:          $(WimpLib).c.os $(WimpLib).c.nd-os
        $(CC) -o $@ $(CCflags) $(WimpLib).c.nd-os

o.pointer:     $(WimpLib).c.pointer $(WimpLib).c.nd-pointer
        $(CC) -o $@ $(CCflags) $(WimpLib).c.nd-pointer

o.print:       $(WimpLib).c.print $(WimpLib).c.nd-print
        $(CC) -o $@ $(CCflags) $(WimpLib).c.nd-print

o.sprite :     $(WimpLib).c.sprite $(WimpLib).c.nd-sprite
        $(CC) -o $@ $(CCflags) $(WimpLib).c.nd-sprite

o.werr :       $(WimpLib).c.werr $(WimpLib).c.nd-werr
        $(CC) -o $@ $(CCflags) $(WimpLib).c.nd-werr

WRAPPED=\
o.drawmod \
o.font \
o.fileicon \
o.os \
o.pointer \
o.print \
o.werr

o.WimpLib:  Makefile \
        $(COLTON_XTRAS) \
        $(PATCHED) \
        $(WRAPPED)
        $(LibFile) $(LibFileflags) \
        $(COLTON_XTRAS) \
        $(PATCHED) \
        $(WRAPPED)
        $(LibFile) -l -s $@ > o.WimpLibLst

# end of Makefile
