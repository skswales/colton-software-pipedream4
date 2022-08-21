@rem Patch the checked-out copy of RISC_OSLib into WimpLib
@rem
@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.
@rem
@rem Copyright (C) 2013-2020 Stuart Swales
@rem
Set flexlib_tmp_dir=..\gitlab\RiscOS\Sources\Toolbox\Toolboxlib\flexlib
Set rlib_tmp_dir=..\gitlab\RiscOS\Sources\Lib\RISC_OSLib\rlib
@rem
@echo Populate WimpLib with unmodifed files from the checked-out copy of RISC_OSLib
@rem
copy %flexlib_tmp_dir%\h\swiextra ..\..\..\swiextra.h
@rem
copy %rlib_tmp_dir%\c\dboxtcol   ..\..\..\dboxtcol.c
copy %rlib_tmp_dir%\c\drawmod    ..\..\..\drawmod.c
copy %rlib_tmp_dir%\c\fileicon   ..\..\..\fileicon.c
copy %rlib_tmp_dir%\c\font       ..\..\..\font.c
copy %rlib_tmp_dir%\c\os         ..\..\..\os.c
copy %rlib_tmp_dir%\c\pointer    ..\..\..\pointer.c
copy %rlib_tmp_dir%\c\print      ..\..\..\print.c
copy %rlib_tmp_dir%\c\werr       ..\..\..\werr.c
@rem
copy %rlib_tmp_dir%\h\akbd       ..\..\..\akbd.h
copy %rlib_tmp_dir%\h\alarm      ..\..\..\alarm.h
copy %rlib_tmp_dir%\h\baricon    ..\..\..\baricon.h
copy %rlib_tmp_dir%\h\bbc        ..\..\..\bbc.h
copy %rlib_tmp_dir%\h\colourpick ..\..\..\colourpick.h
copy %rlib_tmp_dir%\h\coords     ..\..\..\coords.h
copy %rlib_tmp_dir%\h\dbox       ..\..\..\dbox.h
copy %rlib_tmp_dir%\h\dboxtcol   ..\..\..\dboxtcol.h
copy %rlib_tmp_dir%\h\dragasprit ..\..\..\dragasprit.h
copy %rlib_tmp_dir%\h\drawmod    ..\..\..\drawmod.h
copy %rlib_tmp_dir%\h\event      ..\..\..\event.h
copy %rlib_tmp_dir%\h\fileicon   ..\..\..\fileicon.h
@rem copy %rlib_tmp_dir%\h\flex       ..\..\..\flex.h
copy %rlib_tmp_dir%\h\font       ..\..\..\font.h
copy %rlib_tmp_dir%\h\fontlist   ..\..\..\fontlist.h
copy %rlib_tmp_dir%\h\fontselect ..\..\..\fontselect.h
copy %rlib_tmp_dir%\h\help       ..\..\..\help.h
copy %rlib_tmp_dir%\h\menu       ..\..\..\menu.h
copy %rlib_tmp_dir%\h\msgs       ..\..\..\msgs.h
copy %rlib_tmp_dir%\h\os         ..\..\..\os.h
copy %rlib_tmp_dir%\h\pointer    ..\..\..\pointer.h
copy %rlib_tmp_dir%\h\print      ..\..\..\print.h
copy %rlib_tmp_dir%\h\res        ..\..\..\res.h
copy %rlib_tmp_dir%\h\resspr     ..\..\..\resspr.h
copy %rlib_tmp_dir%\h\saveas     ..\..\..\saveas.h
copy %rlib_tmp_dir%\h\sprite     ..\..\..\sprite.h
copy %rlib_tmp_dir%\h\template   ..\..\..\template.h
copy %rlib_tmp_dir%\h\typdat     ..\..\..\typdat.h
copy %rlib_tmp_dir%\h\werr       ..\..\..\werr.h
copy %rlib_tmp_dir%\h\wimpt      ..\..\..\wimpt.h
copy %rlib_tmp_dir%\h\win        ..\..\..\win.h
copy %rlib_tmp_dir%\h\xferrecv   ..\..\..\xferrecv.h
copy %rlib_tmp_dir%\h\xfersend   ..\..\..\xfersend.h
@rem
mkdir ..\..\..\VerIntern
copy %rlib_tmp_dir%\VerIntern\h\messages ..\..\..\VerIntern\messages.h
@rem
@echo Populate WimpLib with files to be patched from the checked-out copy of RISC_OSLib
@rem
copy %flexlib_tmp_dir%\c\flex    ..\..\..\flex.c
@rem
copy %rlib_tmp_dir%\c\bbc        ..\..\..\bbc.c
copy %rlib_tmp_dir%\c\dbox       ..\..\..\dbox.c
copy %rlib_tmp_dir%\c\event      ..\..\..\event.c
@rem copy %rlib_tmp_dir%\c\flex       ..\..\..\flex.c
copy %rlib_tmp_dir%\c\fontlist   ..\..\..\fontlist.c
copy %rlib_tmp_dir%\c\fontselect ..\..\..\fontselect.c
copy %rlib_tmp_dir%\c\menu       ..\..\..\menu.c
copy %rlib_tmp_dir%\c\res        ..\..\..\res.c
copy %rlib_tmp_dir%\c\resspr     ..\..\..\resspr.c
copy %rlib_tmp_dir%\c\win        ..\..\..\win.c
copy %rlib_tmp_dir%\c\wimp       ..\..\..\wimp.c
copy %rlib_tmp_dir%\c\wimpt      ..\..\..\wimpt.c
copy %rlib_tmp_dir%\c\xferrecv   ..\..\..\xferrecv.c
copy %rlib_tmp_dir%\c\xfersend   ..\..\..\xfersend.c
@rem
copy %rlib_tmp_dir%\h\wimp       ..\..\..\wimp.h
@rem
copy %rlib_tmp_dir%\s\swi        ..\..\..\swi.s
@rem
@echo Patch these files in WimpLib so that it is in the WimpLib state for PipeDream
@rem
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < flex.c       ..\..\..\flex.c
@rem
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < bbc.c        ..\..\..\bbc.c
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < dbox.c       ..\..\..\dbox.c
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < event.c      ..\..\..\event.c
@rem ..\..\..\..\external\cs-free\GNU\win32\paatch --unified < flex.c       ..\..\..\flex.c
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < fontlist.c   ..\..\..\fontlist.c
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < fontselect.c ..\..\..\fontselect.c
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < menu.c       ..\..\..\menu.c
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < res.c        ..\..\..\res.c
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < resspr.c     ..\..\..\resspr.c
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < win.c        ..\..\..\win.c
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < wimp.c       ..\..\..\wimp.c
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < wimpt.c      ..\..\..\wimpt.c
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < xferrecv.c   ..\..\..\xferrecv.c
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < xfersend.c   ..\..\..\xfersend.c
@rem
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < wimp.h       ..\..\..\wimp.h
@rem
..\..\..\..\external\cs-free\GNU\win32\paatch --unified < swi.s        ..\..\..\swi.s
