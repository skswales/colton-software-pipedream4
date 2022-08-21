# Patch the checked-out copy of RISC_OSLib into WimpLib

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Copyright (C) 2013-2020 Stuart Swales

flexlib_tmp_dir=../gitlab/RiscOS/Sources/Toolbox/Toolboxlib/flexlib
rlib_tmp_dir=../gitlab/RiscOS/Sources/Lib/RISC_OSLib/rlib
#
echo Populate WimpLib with unmodifed files from the checked-out copy of RISC_OSLib
#
cp -p $flexlib_tmp_dir/h/swiextra ../../../swiextra.h
#
cp -p $rlib_tmp_dir/c/dboxtcol   ../../../dboxtcol.c
cp -p $rlib_tmp_dir/c/drawmod    ../../../drawmod.c
cp -p $rlib_tmp_dir/c/fileicon   ../../../fileicon.c
cp -p $rlib_tmp_dir/c/font       ../../../font.c
cp -p $rlib_tmp_dir/c/os         ../../../os.c
cp -p $rlib_tmp_dir/c/pointer    ../../../pointer.c
cp -p $rlib_tmp_dir/c/print      ../../../print.c
cp -p $rlib_tmp_dir/c/werr       ../../../werr.c
#
cp -p $rlib_tmp_dir/h/akbd       ../../../akbd.h
cp -p $rlib_tmp_dir/h/alarm      ../../../alarm.h
cp -p $rlib_tmp_dir/h/baricon    ../../../baricon.h
cp -p $rlib_tmp_dir/h/bbc        ../../../bbc.h
cp -p $rlib_tmp_dir/h/colourpick ../../../colourpick.h
cp -p $rlib_tmp_dir/h/coords     ../../../coords.h
cp -p $rlib_tmp_dir/h/dbox       ../../../dbox.h
cp -p $rlib_tmp_dir/h/dboxtcol   ../../../dboxtcol.h
cp -p $rlib_tmp_dir/h/dragasprit ../../../dragasprit.h
cp -p $rlib_tmp_dir/h/drawmod    ../../../drawmod.h
cp -p $rlib_tmp_dir/h/event      ../../../event.h
cp -p $rlib_tmp_dir/h/fileicon   ../../../fileicon.h
# cp -p $rlib_tmp_dir/h/flex       ../../../flex.h
cp -p $rlib_tmp_dir/h/font       ../../../font.h
cp -p $rlib_tmp_dir/h/fontlist   ../../../fontlist.h
cp -p $rlib_tmp_dir/h/fontselect ../../../fontselect.h
cp -p $rlib_tmp_dir/h/help       ../../../help.h
cp -p $rlib_tmp_dir/h/menu       ../../../menu.h
cp -p $rlib_tmp_dir/h/msgs       ../../../msgs.h
cp -p $rlib_tmp_dir/h/os         ../../../os.h
cp -p $rlib_tmp_dir/h/pointer    ../../../pointer.h
cp -p $rlib_tmp_dir/h/print      ../../../print.h
cp -p $rlib_tmp_dir/h/res        ../../../res.h
cp -p $rlib_tmp_dir/h/resspr     ../../../resspr.h
cp -p $rlib_tmp_dir/h/saveas     ../../../saveas.h
cp -p $rlib_tmp_dir/h/sprite     ../../../sprite.h
cp -p $rlib_tmp_dir/h/template   ../../../template.h
cp -p $rlib_tmp_dir/h/typdat     ../../../typdat.h
cp -p $rlib_tmp_dir/h/werr       ../../../werr.h
cp -p $rlib_tmp_dir/h/wimpt      ../../../wimpt.h
cp -p $rlib_tmp_dir/h/win        ../../../win.h
cp -p $rlib_tmp_dir/h/xferrecv   ../../../xferrecv.h
cp -p $rlib_tmp_dir/h/xfersend   ../../../xfersend.h
#
mkdir ../../../VerIntern
cp -p $rlib_tmp_dir/VerIntern/h/messages ../../../VerIntern/messages.h
#
echo Populate SKS_ACW with files to be patched from the checked-out copy of RISC_OSLib
#
cp -p $flexlib_tmp_dir/c/flex    ../../../flex.c
#
cp -p $rlib_tmp_dir/c/bbc        ../../../bbc.c
cp -p $rlib_tmp_dir/c/dbox       ../../../dbox.c
cp -p $rlib_tmp_dir/c/event      ../../../event.c
# cp -p $rlib_tmp_dir/c/flex       ../../../flex.c
cp -p $rlib_tmp_dir/c/fontlist   ../../../fontlist.c
cp -p $rlib_tmp_dir/c/fontselect ../../../fontselect.c
cp -p $rlib_tmp_dir/c/menu       ../../../menu.c
cp -p $rlib_tmp_dir/c/res        ../../../res.c
cp -p $rlib_tmp_dir/c/resspr     ../../../resspr.c
cp -p $rlib_tmp_dir/c/win        ../../../win.c
cp -p $rlib_tmp_dir/c/wimp       ../../../wimp.c
cp -p $rlib_tmp_dir/c/wimpt      ../../../wimpt.c
cp -p $rlib_tmp_dir/c/xferrecv   ../../../xferrecv.c
cp -p $rlib_tmp_dir/c/xfersend   ../../../xfersend.c
#
cp -p $rlib_tmp_dir/h/wimp       ../../../wimp.h
#
cp -p $rlib_tmp_dir/s/swi        ../../../swi.s
#
echo Patch these files in WimpLib so that it is in the state for PipeDream
#
pushd ../../..
patch --unified < $OLDPWD/flex.c       flex.c
#
patch --unified < $OLDPWD/bbc.c        bbc.c
patch --unified < $OLDPWD/dbox.c       dbox.c
patch --unified < $OLDPWD/event.c      event.c
# patch --unified < $OLDPWD/flex.c       flex.c
patch --unified < $OLDPWD/fontlist.c   fontlist.c
patch --unified < $OLDPWD/fontselect.c fontselect.c
patch --unified < $OLDPWD/menu.c       menu.c
patch --unified < $OLDPWD/res.c        res.c
patch --unified < $OLDPWD/resspr.c     resspr.c
patch --unified < $OLDPWD/win.c        win.c
patch --unified < $OLDPWD/wimp.c       wimp.c
patch --unified < $OLDPWD/wimpt.c      wimpt.c
patch --unified < $OLDPWD/xferrecv.c   xferrecv.c
patch --unified < $OLDPWD/xfersend.c   xfersend.c
#
patch --unified < $OLDPWD/wimp.h       wimp.h
#
patch --unified < $OLDPWD/swi.s        swi.s
popd
