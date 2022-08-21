# pd.debug (32-bit) - NB profile using debug not release

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Copyright (C) 1989-1998 Colton Software Limited
# Copyright (C) 1998-2015 R W Colton

#TEXTUAL_VERSION_NUMBER=4.54.01
TEXTUAL_VERSION_NUMBER=4.54
# ^^^ Needed for !Boot/!Run building

URD=$.pipedream.trunk

#PD=$(URD).pd
PD=^.^

CMODULES=$(PD).cmodules

PD_EXTERNAL=$(PD).external

LIB_MSUN=$(PD_EXTERNAL).FreeBSD.srcp.lib.msun

WimpLib=$(PD).WimpLib

# Can speed up build (especially when using HostFS on RPCemu on Windows) by using RAM disc
# Uncomment these vvv if you want

# INC_RAM_WimpLib=-IRAM:$.WimpLib

# INC_RAM_PD=-IRAM:$.pd

# Use current set of 32-bit tools on Run$Path and libraries on C$Path

AS=ObjAsm
ASflags_BASE=-I$(CMODULES).Hdr -throwback -stamp -quit -CloseExec
ASflags=$(ASflags_BASE)

CC=cc
CCflags_BASE=-c $(INC_RAM_PD) -I$(PD) $(INC_RAM_WimpLib) -I$(WimpLib) -fah -throwback -DSB_GLOBAL_REGISTER
CCflags=$(CCflags_BASE) -Otime
#
CCflags=$(CCflags_BASE) -Otime -DCHECKING
# Uncomment the above line ^^^ to get some modules compiled with assertion
#
# CCflags=$(CCflags_BASE) -Otime -DUNRELEASED
# Uncomment the above line ^^^ to get some modules compiled with trace info and assertion
#
# Stuart's top tip - just copy over the set of r32.o
# then delete the ones you want compiled with trace info
# You'll probably need / want to rebuild some files:
# o.pdmain as that sets up TRACE_ALLOWED stuff
# o.modules.aligator as that exports some CHECKING functions
#
# CCflags=$(CCflags_BASE) -g
# Uncomment the above line ^^^ to get some modules compiled with debug info
#
# CCflags=$(CCflags_BASE) -g -DUNRELEASED
# Uncomment the above line ^^^ to get some modules compiled with debug info and tracing
#
# CCflags=$(CCflags_BASE) -Otime -DPROFILING
# Uncomment the above line ^^^ for Profiling
# recompile pdmain to enable the Profiler module
# put a profile_on()/off() pair around interesting code

LD=link
AOFLinkflags=-aof -o $@
Linkflags=-aif -o $@
# For Profiling add -d for more symbols; -d is needed for debug info!

PipeDream:   \
    @.!PipeDream.!Boot \
    @.!PipeDream.!Run \
    @.!PipeDream.!RunImage \
    @.!PipeDream.!Sprites \
    @.!PipeDream.!Sprites22 \
    @.!PipeDream.!SpritesR5 \
    @.!PipeDream.Resource.RISC_OS.Messages \
    @.!PipeDream.Resource.RISC_OS.tem_risc \
    @.!PipeDream.Resource.RISC_OS.tem_main \
    @.!PipeDream.Resource.RISC_OS.tem_cht \
    @.!PipeDream.Resource.RISC_OS.tem_bc \
    @.!PipeDream.Resource.RISC_OS.tem_el \
    @.!PipeDream.Resource.RISC_OS.tem_f \
    @.!PipeDream.Resource.RISC_OS.tem_p \
    @.!PipeDream.Resource.RISC_OS.tem_s \
    @.!PipeDream.Resource.RISC_OS.Sprites \
    @.!PipeDream.Resource.RISC_OS.Sprites22

AOF1=\
o.pdmain \
o.blocks \
o.browse \
o.bufedit \
o.colh \
o.commlin \
o.constr \
o.cursmov \
o.dialog \
o.doprint \
o.execs
#
AOF2=\
o.expedit \
o.help \
o.link_ev \
o.lists \
o.markers \
o.mcdiff \
o.numbers \
o.pdriver \
o.pdsearch \
o.progvars
#
AOF3=\
o.reperr \
o.replace \
o.ridialog \
o.riscdraw \
o.riscos \
o.riscmenu \
o.scdraw \
o.slot \
o.slotconv \
o.strings
#
AOF4=\
o.savload \
o.slector \
o.viewio \
o.dtpsave \
o.userIO \
o.version \
o.windvars
#
AOFGR=\
o.pdchart
#
AOFD=
#
#
AOFM1=\
o.modules.aligator \
o.modules.alloc \
o.modules.allocblk \
o.modules.bezier \
o.modules.collect \
o.modules.file \
o.modules.fileutil \
o.modules.funclist \
o.modules.im_cache \
o.modules.im_convert \
o.modules.handlist \
o.modules.mlec
#
AOFM2=\
o.modules.numform \
o.modules.quickblk \
o.modules.report \
o.modules.spell \
o.modules.stringlk \
o.modules.typepack \
o.modules.vsload \
o.modules.wm_event \
o.modules.wm_null \
o.modules.xstring
#
AOFM3=\
o.modules.fontlxtr \
o.modules.tristate \
o.modules.wlalloc
#
AOFMA=\
o.modules.muldivas
#
AOFMEV=\
o.modules.ss_const \
o.modules.ss_date \
o.modules.ev_comp \
o.modules.ev_dcom \
o.modules.ev_docs \
o.modules.ev_eval \
o.modules.ev_exec \
o.modules.ev_fndat \
o.modules.ev_fnfin \
o.modules.ev_fnsta \
o.modules.ev_fnstm \
o.modules.ev_fnstr \
o.modules.ev_func \
o.modules.ev_help \
o.modules.ev_math \
o.modules.ev_matb \
o.modules.ev_matr \
o.modules.ev_mcpx \
o.modules.ev_mtri \
o.modules.ev_name \
o.modules.ev_rpn \
o.modules.ev_tabl \
o.modules.ev_tree \
o.modules.ev_uref \
o.modules.mathxtra \
o.modules.mathxtr2 \
o.modules.mathxtr3 \
o.modules.mathxtr4
#
AOFMGR=\
o.modules.gr_axis2 \
o.modules.gr_axisp \
o.modules.gr_barch \
o.modules.gr_blgal \
o.modules.gr_chart \
o.modules.gr_chtIO \
o.modules.gr_coord \
o.modules.gr_diag \
o.modules.gr_editc \
o.modules.gr_editm \
o.modules.gr_editt \
o.modules.gr_numcv \
o.modules.gr_nodbg \
o.modules.gr_piesg \
o.modules.gr_rdiag \
o.modules.gr_rdia2 \
o.modules.gr_scatc \
o.modules.gr_style \
o.modules.wn_pnpoly
#
AOFMD=\
o.modules.debug \
o.modules.myassert
# o.modules.debug \
# o.modules.myassert
#
#
AOFND=$(AOF1) $(AOF2) $(AOF3) $(AOF4) $(AOFGR) $(AOFM1) $(AOFM2) $(AOFM3) $(AOFMA) $(AOFMEV) $(AOFMGR)
#
AOFDB=$(AOFD) $(AOFMD)
#
# United Kingdom image
#
#       Insert -d for debug symbols, -v (multiple times) for interesting output
#
@.!PipeDream.!RunImage: @.Makefile \
        $(AOFND) $(AOFDB) \
        $(WimpLib).r32.o.WimpLib \
        C:o.Stubs
        @Echo If the link step fails, please look in o.GB_MapIm for the error
        $(LD) -v $(Linkflags) -base &8000 \
        $(AOFND) $(AOFDB) \
        $(WimpLib).r32.o.WimpLib \
        C:o.Stubs \
        -map -xref > @.o.GB_MapIm 2>&1 -symbols @.o.GB_SymIm
        |SetType $@ Absolute
        @Info $@
#
#       C:o.AnsiLib
# You may wish to link with AnsiLib for better profiling resolution
#
o.aof_inter: Makefile
        $(LD) $(AOFLinkflags) o.pdmain C:o.Stubs

o.aof_GB:   \
        @.Makefile \
        $(AOFND) $(AOFDB) \
        $(WimpLib).r32.o.WimpLib \
        C:o.Stubs
        $(LD) -v $(AOFLinkflags) \
        $(AOFND) $(AOFDB) \
        $(WimpLib).r32.o.WimpLib \
        C:o.Stubs \
        -map -xref > @.o.GB_Map00 2>&1 -symbols @.o.GB_Sym00
#
# working copy of the United Kingdom release sprites file
#
@.!PipeDream.!Sprites:   \
             $(PD).utils.GB.spr_fi24
        Copy $(PD).utils.GB.spr_fi24 $@ ~C FV

@.!PipeDream.!Sprites22:   \
             $(PD).utils.GB.spr_fi22
        Copy $(PD).utils.GB.spr_fi22 $@ ~C FV

@.!PipeDream.!SpritesR5:   \
             $(PD).utils.GB.spr_fiR5
        Copy $(PD).utils.GB.spr_fiR5 $@ ~C FV

@.!PipeDream.Resource.RISC_OS.Sprites:   \
             utils.GB.s24_mrg
        Copy @.utils.GB.s24_mrg $@ ~C FV

@.!PipeDream.Resource.RISC_OS.Sprites22:   \
             utils.GB.s22_mrg
        Copy @.utils.GB.s22_mrg $@ ~C FV
#
# and the template files too
#
@.!PipeDream.Resource.RISC_OS.tem_risc:   \
             $(PD).utils.GB.tem_risc
        Copy $(PD).utils.GB.tem_risc $@ ~C FV

@.!PipeDream.Resource.RISC_OS.tem_main:   \
             $(PD).utils.GB.tem_main
        Copy $(PD).utils.GB.tem_main $@ ~C FV

@.!PipeDream.Resource.RISC_OS.tem_cht:   \
             $(PD).utils.GB.tem_cht
        Copy $(PD).utils.GB.tem_cht $@ ~C FV

@.!PipeDream.Resource.RISC_OS.tem_bc:   \
             $(PD).utils.GB.tem_bc
        Copy $(PD).utils.GB.tem_bc $@ ~C FV

@.!PipeDream.Resource.RISC_OS.tem_el:   \
             $(PD).utils.GB.tem_el
        Copy $(PD).utils.GB.tem_el $@ ~C FV

@.!PipeDream.Resource.RISC_OS.tem_f:   \
             $(PD).utils.GB.tem_f
        Copy $(PD).utils.GB.tem_f $@ ~C FV

@.!PipeDream.Resource.RISC_OS.tem_p:   \
             $(PD).utils.GB.tem_p
        Copy $(PD).utils.GB.tem_p $@ ~C FV

@.!PipeDream.Resource.RISC_OS.tem_s:   \
             $(PD).utils.GB.tem_s
        Copy $(PD).utils.GB.tem_s $@ ~C FV
#
# and the message file too
#
@.!PipeDream.Resource.RISC_OS.Messages:   \
             utils.GB.Messages
        Copy @.utils.GB.Messages $@ ~C FV

@.!PipeDream.!Boot: \
        $(PD).Build.Tools.BootRunGen \
        @.Makefile
        Basic -quit $(PD).Build.Tools.BootRunGen -output $@ -version $(TEXTUAL_VERSION_NUMBER) -d32 -boot

@.!PipeDream.!Run: \
        $(PD).Build.Tools.BootRunGen \
        @.Makefile
        Basic -quit $(PD).Build.Tools.BootRunGen -output $@ -version $(TEXTUAL_VERSION_NUMBER) -d32

@.!RunDB: \
        $(PD).Build.Tools.BootRunGen \
        @.Makefile
        Basic -quit $(PD).Build.Tools.BootRunGen -output $@ -version $(TEXTUAL_VERSION_NUMBER) -d32 -alternate

@.!RunND: \
        $(PD).Build.Tools.BootRunGen \
        @.Makefile
        Basic -quit $(PD).Build.Tools.BootRunGen -output $@ -version $(TEXTUAL_VERSION_NUMBER) -d32 -alternate

# ####################################################################
#
# C modules static dependencies
#
o.Modules.aligator:   $(CMODULES).c.aligator
        $(CC) -o $@ $(CCflags) $(CMODULES).c.aligator
o.Modules.alloc:      $(CMODULES).c.alloc
        $(CC) -o $@ $(CCflags) $(CMODULES).c.alloc
o.Modules.allocblk:   $(CMODULES).c.allocblk
        $(CC) -o $@ $(CCflags) $(CMODULES).c.allocblk
o.Modules.bezier:     $(CMODULES).c.bezier
        $(CC) -o $@ $(CCflags) $(CMODULES).c.bezier
o.Modules.collect:    $(CMODULES).c.collect
        $(CC) -o $@ $(CCflags) $(CMODULES).c.collect
o.Modules.debug:      $(CMODULES).c.debug
        $(CC) -o $@ $(CCflags) $(CMODULES).c.debug
o.Modules.ev_comp:    $(CMODULES).c.ev_comp
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_comp
o.Modules.ev_dcom:    $(CMODULES).c.ev_dcom
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_dcom
o.Modules.ev_docs:    $(CMODULES).c.ev_docs
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_docs
o.Modules.ev_eval:    $(CMODULES).c.ev_eval
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_eval
o.Modules.ev_exec:    $(CMODULES).c.ev_exec
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_exec
o.Modules.ev_fndat:    $(CMODULES).c.ev_fndat
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_fndat
o.Modules.ev_fnfin:    $(CMODULES).c.ev_fnfin
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_fnfin
o.Modules.ev_fnsta:    $(CMODULES).c.ev_fnsta
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_fnsta
o.Modules.ev_fnstm:    $(CMODULES).c.ev_fnstm
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_fnstm
o.Modules.ev_fnstr:    $(CMODULES).c.ev_fnstr
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_fnstr
o.Modules.ev_func:    $(CMODULES).c.ev_func
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_func
o.Modules.ev_help:    $(CMODULES).c.ev_help
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_help
o.Modules.ev_math:    $(CMODULES).c.ev_math
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_math
o.Modules.ev_matb:    $(CMODULES).c.ev_matb
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_matb
o.Modules.ev_matr:    $(CMODULES).c.ev_matr
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_matr
o.Modules.ev_mcpx:    $(CMODULES).c.ev_mcpx
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_mcpx
o.Modules.ev_mtri:    $(CMODULES).c.ev_mtri
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_mtri
o.Modules.ev_name:    $(CMODULES).c.ev_name
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_name
o.Modules.ev_rpn:     $(CMODULES).c.ev_rpn
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_rpn
o.Modules.ev_tabl:    $(CMODULES).c.ev_tabl
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_tabl
o.Modules.ev_tree:    $(CMODULES).c.ev_tree
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_tree
o.Modules.ev_uref:    $(CMODULES).c.ev_uref
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ev_uref
o.Modules.file:       $(CMODULES).c.file
        $(CC) -o $@ $(CCflags) $(CMODULES).c.file
o.Modules.fileutil:   $(CMODULES).c.fileutil
        $(CC) -o $@ $(CCflags) $(CMODULES).c.fileutil
o.Modules.funclist:   $(CMODULES).c.funclist
        $(CC) -o $@ $(CCflags) $(CMODULES).c.funclist
o.Modules.gr_axis2:   $(CMODULES).c.gr_axis2
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_axis2
o.Modules.gr_axisp:   $(CMODULES).c.gr_axisp
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_axisp
o.Modules.gr_barch:   $(CMODULES).c.gr_barch
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_barch
o.Modules.gr_blgal:   $(CMODULES).c.gr_blgal
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_blgal
o.Modules.gr_chart:   $(CMODULES).c.gr_chart
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_chart
o.Modules.gr_chtIO:   $(CMODULES).c.gr_chtIO
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_chtIO
o.Modules.gr_coord:   $(CMODULES).c.gr_coord
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_coord
o.Modules.gr_diag:    $(CMODULES).c.gr_diag
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_diag
o.Modules.gr_editc:   $(CMODULES).c.gr_editc
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_editc
o.Modules.gr_editm:   $(CMODULES).c.gr_editm
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_editm
o.Modules.gr_editt:   $(CMODULES).c.gr_editt
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_editt
o.Modules.gr_nodbg:   $(CMODULES).c.gr_nodbg
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_nodbg
o.Modules.gr_numcv:   $(CMODULES).c.gr_numcv
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_numcv
o.Modules.gr_piesg:    $(CMODULES).c.gr_piesg
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_piesg
o.Modules.gr_rdiag:    $(CMODULES).c.gr_rdiag
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_rdiag
o.Modules.gr_rdia2:    $(CMODULES).c.gr_rdia2
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_rdia2
o.Modules.gr_scatc:    $(CMODULES).c.gr_scatc
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_scatc
o.Modules.gr_style:    $(CMODULES).c.gr_style
        $(CC) -o $@ $(CCflags) $(CMODULES).c.gr_style
o.Modules.handlist:   $(CMODULES).c.handlist
        $(CC) -o $@ $(CCflags) $(CMODULES).c.handlist
o.Modules.im_cache:   $(CMODULES).c.im_cache
        $(CC) -o $@ $(CCflags) $(CMODULES).c.im_cache
o.Modules.im_convert: $(CMODULES).c.im_convert
        $(CC) -o $@ $(CCflags) $(CMODULES).c.im_convert
o.Modules.list:       $(CMODULES).c.list
        $(CC) -o $@ $(CCflags) $(CMODULES).c.list
o.Modules.mathxtra:   $(CMODULES).c.mathxtra
        $(CC) -o $@ $(CCflags) $(CMODULES).c.mathxtra
o.Modules.mathxtr2:   $(CMODULES).c.mathxtr2
        $(CC) -o $@ $(CCflags) $(CMODULES).c.mathxtr2
o.Modules.mathxtr3:   $(CMODULES).c.mathxtr3
        $(CC) -o $@ $(CCflags) $(CMODULES).c.mathxtr3
o.Modules.mathxtr4:   $(CMODULES).c.mathxtr4
        $(CC) -o $@ $(CCflags) $(CMODULES).c.mathxtr4
o.Modules.mlec:       $(CMODULES).c.mlec
        $(CC) -o $@ $(CCflags) $(CMODULES).c.mlec
o.Modules.muldiv:     $(CMODULES).c.muldiv
        $(CC) -o $@ $(CCflags) $(CMODULES).c.muldiv
o.Modules.myassert:   $(CMODULES).c.myassert
        $(CC) -o $@ $(CCflags) $(CMODULES).c.myassert
o.Modules.numform:    $(CMODULES).c.numform
        $(CC) -o $@ $(CCflags) $(CMODULES).c.numform
o.Modules.quickblk:   $(CMODULES).c.quickblk
        $(CC) -o $@ $(CCflags) $(CMODULES).c.quickblk
o.Modules.report:     $(CMODULES).c.report
        $(CC) -o $@ $(CCflags) $(CMODULES).c.report
o.Modules.spell:      $(CMODULES).c.spell
        $(CC) -o $@ $(CCflags) $(CMODULES).c.spell
o.Modules.ss_const:   $(CMODULES).c.ss_const
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ss_const
o.Modules.ss_date:   $(CMODULES).c.ss_date
        $(CC) -o $@ $(CCflags) $(CMODULES).c.ss_date
o.Modules.stringlk:   $(CMODULES).c.stringlk
        $(CC) -o $@ $(CCflags) $(CMODULES).c.stringlk
o.Modules.typepack:   $(CMODULES).c.typepack
        $(CC) -o $@ $(CCflags) $(CMODULES).c.typepack
o.Modules.vsload:     $(CMODULES).c.vsload
        $(CC) -o $@ $(CCflags) $(CMODULES).c.vsload
o.Modules.wm_event:   $(CMODULES).c.wm_event
        $(CC) -o $@ $(CCflags) $(CMODULES).c.wm_event
o.Modules.wm_null:    $(CMODULES).c.wm_null
        $(CC) -o $@ $(CCflags) $(CMODULES).c.wm_null
o.Modules.xstring:    $(CMODULES).c.xstring
        $(CC) -o $@ $(CCflags) $(CMODULES).c.xstring
#
# RISCOS specific
#
o.Modules.fontlxtr:   $(CMODULES).riscos.c.fontlxtr
        $(CC) -o $@ $(CCflags) $(CMODULES).riscos.c.fontlxtr
o.Modules.ho_dll:     $(CMODULES).riscos.c.ho_dll
        $(CC) -o $@ $(CCflags) $(CMODULES).riscos.c.ho_dll
o.Modules.ho_sqush:   $(CMODULES).riscos.c.ho_sqush
        $(CC) -o $@ $(CCflags) $(CMODULES).riscos.c.ho_sqush
o.Modules.tristate:   $(CMODULES).riscos.c.tristate
        $(CC) -o $@ $(CCflags) $(CMODULES).riscos.c.tristate
o.Modules.wlalloc:    $(CMODULES).riscos.c.wlalloc
        $(CC) -o $@ $(CCflags) $(CMODULES).riscos.c.wlalloc
#
# FreeBSD lib/msun
#
o.Modules.e_lgamma_r: $(LIB_MSUN).src.c.e_lgamma_r
        $(CC) -o $@ $(CCflags) $(LIB_MSUN).src.c.e_lgamma_r
o.Modules.k_cos:    $(LIB_MSUN).src.c.k_cos
        $(CC) -o $@ $(CCflags) $(LIB_MSUN).src.c.k_cos
o.Modules.k_sin:    $(LIB_MSUN).src.c.k_sin
        $(CC) -o $@ $(CCflags) $(LIB_MSUN).src.c.k_sin
#
# OtherFree
#
o.Modules.wn_pnpoly:	$(PD_EXTERNAL).OtherFree.c.wn_pnpoly
        $(CC) -o $@ $(CCflags) $(PD_EXTERNAL).OtherFree.c.wn_pnpoly
#
# ARM assembler modules static dependencies
#
o.Modules.muldivas:   $(CMODULES).riscos.c.muldivas
        $(AS) -to $@ $(ASflags) -from $(CMODULES).riscos.c.muldivas
#
# C modules extra dependencies (comment out appropriately during severe hacks)
#
o.Modules.aligator :   $(CMODULES).h.aligator
o.Modules.alloc :      $(CMODULES).h.alloc
o.Modules.allocblk :   $(CMODULES).h.allocblk
o.Modules.bezier :     $(CMODULES).h.bezier
o.Modules.collect :    $(CMODULES).h.collect
o.Modules.debug :      $(CMODULES).h.debug
o.Modules.ev_comp :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_dcom :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_docs :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_eval :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_exec :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_fndat :   $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_fnfin :   $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_fnsta :   $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_fnstm :   $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_fnstr :   $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_func :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_help :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_math :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_matb :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_matr :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_mcpx :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_mtri :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_name :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_rpn :     $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_tabl :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_tree :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.ev_uref :    $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.file :       $(CMODULES).h.file
o.Modules.fileutil :   $(CMODULES).h.file
o.Modules.funclist :   $(CMODULES).h.funclist
o.Modules.gr_axisp :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_barch :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_blgal :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_chart :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_chtIO :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_coord :   $(CMODULES).h.gr_coord
o.Modules.gr_diag :    $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_editc :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_editm :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_editt :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_nodbg :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_numcv :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_piesg :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_rdiag :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_rdia2 :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_scatc :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.gr_style :   $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari
o.Modules.handlist :   $(CMODULES).h.handlist
o.Modules.im_cache :   $(CMODULES).h.im_cache
o.Modules.list :       $(CMODULES).h.list
o.Modules.mathxtra :   $(CMODULES).h.mathxtra
o.Modules.mathxtr2 :   $(CMODULES).h.mathxtr2
o.Modules.mathxtr3 :   $(CMODULES).h.mathxtr3
o.Modules.mathxtr4 :   $(CMODULES).h.mathxtr4
o.Modules.mlec :       $(CMODULES).h.mlec
o.Modules.monotime :   $(CMODULES).h.monotime
o.Modules.muldiv :     $(CMODULES).h.muldiv
o.Modules.myassert :   $(CMODULES).h.myassert
o.Modules.numform :    $(CMODULES).h.numform
o.Modules.quickblk :   $(CMODULES).h.quickblk
o.Modules.spell :      $(CMODULES).h.spell
o.Modules.ss_const :   $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const
o.Modules.stringlk :   $(CMODULES).h.stringlk
o.Modules.typepack :   $(CMODULES).h.typepack
o.Modules.vsload :     $(CMODULES).h.vsload
o.Modules.wm_null :    $(CMODULES).h.wm_event
o.Modules.xstring :    $(CMODULES).h.xstring
#
o.Modules.fontlxtr :   $(CMODULES).riscos.h.fontlxtr
o.Modules.ho_dll :     $(CMODULES).riscos.h.ho_dll
o.Modules.ho_sqush :   $(CMODULES).riscos.h.ho_sqush
o.Modules.tristate :   $(CMODULES).riscos.h.tristate
o.Modules.wlalloc :    $(CMODULES).riscos.h.wlalloc
#
# end of Makefile (cmodules include)
#
# ####################################################################
#
# PipeDream objects
#
o.blocks:   $(PD).c.blocks
        $(CC) -o $@ $(CCflags) $(PD).c.blocks
o.browse:   $(PD).c.browse
        $(CC) -o $@ $(CCflags) $(PD).c.browse
o.bufedit:   $(PD).c.bufedit
        $(CC) -o $@ $(CCflags) $(PD).c.bufedit
o.colh:   $(PD).c.colh
        $(CC) -o $@ $(CCflags) $(PD).c.colh
o.commlin:   $(PD).c.commlin
        $(CC) -o $@ $(CCflags) $(PD).c.commlin
o.constr:   $(PD).c.constr
        $(CC) -o $@ $(CCflags) $(PD).c.constr
o.cursmov:   $(PD).c.cursmov
        $(CC) -o $@ $(CCflags) $(PD).c.cursmov
o.dialog:   $(PD).c.dialog
        $(CC) -o $@ $(CCflags) $(PD).c.dialog
o.doprint:   $(PD).c.doprint
        $(CC) -o $@ $(CCflags) $(PD).c.doprint
o.dtpsave:   $(PD).c.dtpsave
        $(CC) -o $@ $(CCflags) $(PD).c.dtpsave
o.execs:   $(PD).c.execs
        $(CC) -o $@ $(CCflags) $(PD).c.execs
o.expedit:   $(PD).c.expedit
        $(CC) -o $@ $(CCflags) $(PD).c.expedit
o.help:   $(PD).c.help
        $(CC) -o $@ $(CCflags) $(PD).c.help
o.lists:   $(PD).c.lists
        $(CC) -o $@ $(CCflags) $(PD).c.lists
o.link_ev:   $(PD).c.link_ev
        $(CC) -o $@ $(CCflags) $(PD).c.link_ev
o.markers:   $(PD).c.markers
        $(CC) -o $@ $(CCflags) $(PD).c.markers
o.mcdiff:   $(PD).c.mcdiff
        $(CC) -o $@ $(CCflags) $(PD).c.mcdiff
o.numbers:   $(PD).c.numbers
        $(CC) -o $@ $(CCflags) $(PD).c.numbers
o.pdchart:   $(PD).c.pdchart
        $(CC) -o $@ $(CCflags) $(PD).c.pdchart
o.pdmain:   $(PD).c.pdmain
        $(CC) -o $@ $(CCflags) $(PD).c.pdmain
o.pdriver:   $(PD).c.pdriver
        $(CC) -o $@ $(CCflags) $(PD).c.pdriver
o.pdsearch:   $(PD).c.pdsearch
        $(CC) -o $@ $(CCflags) $(PD).c.pdsearch
o.progvars:   $(PD).c.progvars
        $(CC) -o $@ $(CCflags) $(PD).c.progvars
o.prototyp:   $(PD).c.prototyp
        $(CC) -o $@ $(CCflags) $(PD).c.prototyp
o.reperr:   $(PD).c.reperr
        $(CC) -o $@ $(CCflags) $(PD).c.reperr
o.replace:   $(PD).c.replace
        $(CC) -o $@ $(CCflags) $(PD).c.replace
o.ridialog:   $(PD).c.ridialog
        $(CC) -o $@ $(CCflags) $(PD).c.ridialog
o.riscdraw:   $(PD).c.riscdraw
        $(CC) -o $@ $(CCflags) $(PD).c.riscdraw
o.riscmenu:   $(PD).c.riscmenu
        $(CC) -o $@ $(CCflags) $(PD).c.riscmenu
o.riscos:   $(PD).c.riscos
        $(CC) -o $@ $(CCflags) $(PD).c.riscos
o.savload:   $(PD).c.savload
        $(CC) -o $@ $(CCflags) $(PD).c.savload
o.scdraw:   $(PD).c.scdraw
        $(CC) -o $@ $(CCflags) $(PD).c.scdraw
o.slector:   $(PD).c.slector
        $(CC) -o $@ $(CCflags) $(PD).c.slector
o.slot:   $(PD).c.slot
        $(CC) -o $@ $(CCflags) $(PD).c.slot
o.slotconv:   $(PD).c.slotconv
        $(CC) -o $@ $(CCflags) $(PD).c.slotconv
o.strings:   $(PD).c.strings
        $(CC) -o $@ $(CCflags) $(PD).c.strings
o.userIO:   $(PD).c.userIO
        $(CC) -o $@ $(CCflags) $(PD).c.UserIO
o.version:   $(PD).c.version
        $(CC) -o $@ $(CCflags) $(PD).c.version
o.viewio:   $(PD).c.viewio
        $(CC) -o $@ $(CCflags) $(PD).c.viewio
o.windvars:   $(PD).c.windvars
        $(CC) -o $@ $(CCflags) $(PD).c.windvars

# extra dependencies

o.strings: $(PD).h.strings
o.reperr:  $(PD).h.errdef

o.link_ev o.slotconv: $(CMODULES).h.ev_eval $(CMODULES).h.ev_evali $(CMODULES).h.ss_const

o.pdchart: $(PD).h.pdchart
o.pdchart: $(CMODULES).h.gr_chart $(CMODULES).h.gr_chari

###############################################################################
#
#                       M e s s a g e   f i l e s
#
###############################################################################

utils.GB.Messages:   \
        utils.GB.t_reperr \
        utils.GB.t_strings \
        $(WimpLib).GB.msg_risc \
        $(CMODULES).GB.msg.gr_chart \
        $(CMODULES).GB.msg.mlec
        print  { > $@ } utils.GB.t_reperr
        print { >> $@ } utils.GB.t_strings
        print { >> $@ } $(WimpLib).GB.msg_risc
        print { >> $@ } $(CMODULES).GB.msg.gr_chart
        print { >> $@ } $(CMODULES).GB.msg.mlec
        SetType $@ Text

utils.GB.t_reperr:       utils.GB.u_reperr
        Run <Prefix$Dir>.utils.GB.u_reperr > $@

utils.GB.t_strings:      utils.GB.u_strings
        Run <Prefix$Dir>.utils.GB.u_strings > $@

utils.GB.u_reperr:   \
        utils.GB.o.reperr \
        C:o.Stubs
        Link -o $@ -aif \
        utils.GB.o.reperr \
        C:o.Stubs

utils.GB.u_strings:   \
        utils.GB.o.strings \
        C:o.Stubs
        Link -o $@ -aif \
        utils.GB.o.strings \
        C:o.Stubs

utils.GB.o.reperr:   $(PD).h.personal $(PD).h.errdef
utils.GB.o.reperr:   $(PD).c.reperr
        @CDir @.utils
        @CDir @.utils.GB
        @CDir @.utils.GB.o
        $(CC) -o $@ $(CCflags_BASE) -DMAKE_MESSAGE_FILE $(PD).c.reperr

utils.GB.o.strings:   $(PD).h.personal
utils.GB.o.strings:   $(PD).c.strings
        @CDir @.utils
        @CDir @.utils.GB
        @CDir @.utils.GB.o
        $(CC) -o $@ $(CCflags_BASE) -DMAKE_MESSAGE_FILE $(PD).c.strings

###############################################################################
#
#                       S p r i t e   f i l e s
#
###############################################################################

utils.GB.s22_mrg:   \
        $(PD).utils.GB.spr_fi22 \
        $(PD).utils.GB.spr_pd22  \
        $(PD).utils.GB.spr_pd \
        $(PD).utils.GB.spr_ch22 \
        $(PD).utils.GB.spr_chart \
        $(WimpLib).GB.spr_risc
        ChangeDynamicArea -SpriteSize 256k
        SNew
        SLoad   $(PD).utils.GB.spr_fi22
       -SDelete sm!pipedream
       -SDelete file_dde
       -SDelete small_dde
       -SDelete file_d21
       -SDelete small_d21
       -SDelete file_dfe
        SMerge  $(PD).utils.GB.spr_pd22
        SMerge  $(PD).utils.GB.spr_pd
        SMerge  $(PD).utils.GB.spr_ch22
        SMerge  $(PD).utils.GB.spr_chart
        SMerge  $(WimpLib).GB.spr_risc
       -SInfo
       -SList
       @CDir    @.utils
       @CDir    @.utils.GB
        SSave   $@
       -SNew
       -ChangeDynamicArea -SpriteSize 32k

utils.GB.s24_mrg:   \
        $(PD).utils.GB.spr_fi24 \
        $(PD).utils.GB.spr_pd24  \
        $(PD).utils.GB.spr_pd \
        $(PD).utils.GB.spr_ch24  \
        $(PD).utils.GB.spr_chart \
        $(WimpLib).GB.spr_risc
        ChangeDynamicArea -SpriteSize 256k
        SNew
        SLoad   $(PD).utils.GB.spr_fi24
       -SDelete sm!pipedream
       -SDelete file_dde
       -SDelete small_dde
       -SDelete file_d21
       -SDelete small_d21
       -SDelete file_dfe
        SMerge  $(PD).utils.GB.spr_pd24
        SMerge  $(PD).utils.GB.spr_pd
        SMerge  $(PD).utils.GB.spr_ch24
        SMerge  $(PD).utils.GB.spr_chart
        SMerge  $(WimpLib).GB.spr_risc
       -SInfo
       -SList
       @CDir    @.utils
       @CDir    @.utils.GB
        SSave   $@
       -SNew
       -ChangeDynamicArea -SpriteSize 32k

# end of Makefile
