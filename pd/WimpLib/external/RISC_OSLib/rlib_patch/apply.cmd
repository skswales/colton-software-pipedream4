@rem Patch the checked-out copy of RISC_OSLib into WimpLib
@rem
@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.
@rem
@rem Copyright (C) 2013-2016 Stuart Swales
@rem
Set flexlib_tmp_dir=..\castle\RiscOS\Sources\Toolbox\Libs\flexlib
Set rlib_tmp_dir=..\castle\RiscOS\Sources\Lib\RISC_OSLib\rlib
@rem
@echo Populate WimpLib with unmodifed files from the checked-out copy of RISC_OSLib
@rem
cp -p %flexlib_tmp_dir%\h\swiextra ..\..\..\swiextra.h
@rem
cp -p %rlib_tmp_dir%\c\colourtran ..\..\..\colourtran.c
cp -p %rlib_tmp_dir%\c\dboxtcol   ..\..\..\dboxtcol.c
cp -p %rlib_tmp_dir%\c\drawmod    ..\..\..\drawmod.c
cp -p %rlib_tmp_dir%\c\fileicon   ..\..\..\fileicon.c
cp -p %rlib_tmp_dir%\c\font       ..\..\..\font.c
cp -p %rlib_tmp_dir%\c\os         ..\..\..\os.c
cp -p %rlib_tmp_dir%\c\pointer    ..\..\..\pointer.c
cp -p %rlib_tmp_dir%\c\print      ..\..\..\print.c
cp -p %rlib_tmp_dir%\c\werr       ..\..\..\werr.c
@rem
cp -p %rlib_tmp_dir%\h\akbd       ..\..\..\akbd.h
cp -p %rlib_tmp_dir%\h\alarm      ..\..\..\alarm.h
cp -p %rlib_tmp_dir%\h\baricon    ..\..\..\baricon.h
cp -p %rlib_tmp_dir%\h\bbc        ..\..\..\bbc.h
cp -p %rlib_tmp_dir%\h\colourpick ..\..\..\colourpick.h
cp -p %rlib_tmp_dir%\h\colourtran ..\..\..\colourtran.h
cp -p %rlib_tmp_dir%\h\coords     ..\..\..\coords.h
cp -p %rlib_tmp_dir%\h\dbox       ..\..\..\dbox.h
cp -p %rlib_tmp_dir%\h\dboxtcol   ..\..\..\dboxtcol.h
cp -p %rlib_tmp_dir%\h\dragasprit ..\..\..\dragasprit.h
cp -p %rlib_tmp_dir%\h\drawmod    ..\..\..\drawmod.h
cp -p %rlib_tmp_dir%\h\event      ..\..\..\event.h
cp -p %rlib_tmp_dir%\h\fileicon   ..\..\..\fileicon.h
@rem cp -p %rlib_tmp_dir%\h\flex       ..\..\..\flex.h
cp -p %rlib_tmp_dir%\h\font       ..\..\..\font.h
cp -p %rlib_tmp_dir%\h\fontlist   ..\..\..\fontlist.h
cp -p %rlib_tmp_dir%\h\fontselect ..\..\..\fontselect.h
cp -p %rlib_tmp_dir%\h\help       ..\..\..\help.h
cp -p %rlib_tmp_dir%\h\menu       ..\..\..\menu.h
cp -p %rlib_tmp_dir%\h\msgs       ..\..\..\msgs.h
cp -p %rlib_tmp_dir%\h\os         ..\..\..\os.h
cp -p %rlib_tmp_dir%\h\pointer    ..\..\..\pointer.h
cp -p %rlib_tmp_dir%\h\print      ..\..\..\print.h
cp -p %rlib_tmp_dir%\h\res        ..\..\..\res.h
cp -p %rlib_tmp_dir%\h\resspr     ..\..\..\resspr.h
cp -p %rlib_tmp_dir%\h\saveas     ..\..\..\saveas.h
cp -p %rlib_tmp_dir%\h\sprite     ..\..\..\sprite.h
cp -p %rlib_tmp_dir%\h\template   ..\..\..\template.h
cp -p %rlib_tmp_dir%\h\typdat     ..\..\..\typdat.h
cp -p %rlib_tmp_dir%\h\werr       ..\..\..\werr.h
cp -p %rlib_tmp_dir%\h\wimpt      ..\..\..\wimpt.h
cp -p %rlib_tmp_dir%\h\win        ..\..\..\win.h
cp -p %rlib_tmp_dir%\h\xferrecv   ..\..\..\xferrecv.h
cp -p %rlib_tmp_dir%\h\xfersend   ..\..\..\xfersend.h
@rem
mkdir ..\..\..\VerIntern
cp -p %rlib_tmp_dir%\VerIntern\h\messages ..\..\..\VerIntern\messages.h
@rem
@echo Populate WimpLib with files to be patched from the checked-out copy of RISC_OSLib
@rem
cp -p %flexlib_tmp_dir%\c\flex    ..\..\..\flex.c
@rem
cp -p %rlib_tmp_dir%\c\bbc        ..\..\..\bbc.c
cp -p %rlib_tmp_dir%\c\dbox       ..\..\..\dbox.c
cp -p %rlib_tmp_dir%\c\event      ..\..\..\event.c
@rem cp -p %rlib_tmp_dir%\c\flex       ..\..\..\flex.c
cp -p %rlib_tmp_dir%\c\fontlist   ..\..\..\fontlist.c
cp -p %rlib_tmp_dir%\c\fontselect ..\..\..\fontselect.c
cp -p %rlib_tmp_dir%\c\menu       ..\..\..\menu.c
cp -p %rlib_tmp_dir%\c\res        ..\..\..\res.c
cp -p %rlib_tmp_dir%\c\resspr     ..\..\..\resspr.c
cp -p %rlib_tmp_dir%\c\win        ..\..\..\win.c
cp -p %rlib_tmp_dir%\c\wimp       ..\..\..\wimp.c
cp -p %rlib_tmp_dir%\c\wimpt      ..\..\..\wimpt.c
cp -p %rlib_tmp_dir%\c\xferrecv   ..\..\..\xferrecv.c
cp -p %rlib_tmp_dir%\c\xfersend   ..\..\..\xfersend.c
@rem
cp -p %rlib_tmp_dir%\h\wimp       ..\..\..\wimp.h
@rem
cp -p %rlib_tmp_dir%\s\swi        ..\..\..\swi.s
@rem
@echo Patch these files in WimpLib so that it is in the WimpLib state for PipeDream
@rem
gnu-paatch --unified < flex.c       ..\..\..\flex.c
@rem
gnu-paatch --unified < bbc.c        ..\..\..\bbc.c
gnu-paatch --unified < dbox.c       ..\..\..\dbox.c
gnu-paatch --unified < event.c      ..\..\..\event.c
@rem gnu-paatch --unified < flex.c       ..\..\..\flex.c
gnu-paatch --unified < fontlist.c   ..\..\..\fontlist.c
gnu-paatch --unified < fontselect.c ..\..\..\fontselect.c
gnu-paatch --unified < menu.c       ..\..\..\menu.c
gnu-paatch --unified < res.c        ..\..\..\res.c
gnu-paatch --unified < resspr.c     ..\..\..\resspr.c
gnu-paatch --unified < win.c        ..\..\..\win.c
gnu-paatch --unified < wimp.c       ..\..\..\wimp.c
gnu-paatch --unified < wimpt.c      ..\..\..\wimpt.c
gnu-paatch --unified < xferrecv.c   ..\..\..\xferrecv.c
gnu-paatch --unified < xfersend.c   ..\..\..\xfersend.c
@rem
gnu-paatch --unified < wimp.h       ..\..\..\wimp.h
@rem
gnu-paatch --unified < swi.s        ..\..\..\swi.s
