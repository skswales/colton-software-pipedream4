PipeDream 4 Releases
--------------------

As a number of the earlier versions did not have git tags corresponding to the svn tags, I've had to add them by hand.

Unfortunately this means that the earlier versions have risen to the top of the Release pages!

The current version is 4.60/02, you can find it by digging...




ReadMe for PipeDream 4 Build
----------------------------

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.

Copyright (C) 2013-2022 Stuart Swales

Prerequisites
-------------

Acorn C/C++ Release 25 or later (!Amu, C compiler, headers, libraries, linker).

InfoZip (install with PackMan) for creating a zip of the final build.

A RISC OS git client (install Simplegit-Libgit2 with PackMan) to download a copy of RISC_OSLib.

patch (install with PackMan) to patch that copy of RISC_OSLib for use with
PipeDream.


First-time configuration and build
----------------------------------

Ensure that the system has 'seen' AcornC/C++ and set that environment.

Run !!!Boot to set up the PipeDream build environment.

Run configure to create the build directories.

You need to then check out and build a pruned and patched variant of
RISC_OSLib with PipeDream-specific build options. See ^.WimpLib.ReadMe and
^.WimpLib.external.RISC_OSLib.ReadMe for instructions on how to do this.

Run !Amu.

Drag 'r32.Makefile' to !Amu... and wait...  A PipeDream build takes
about 16 minutes on a SA RISC PC.  Building on an ARMX6 takes about
one minute.


Subsequent builds
-----------------

After any reboot, you will need to:

Ensure that the system has 'seen' AcornC/C++ and set that environment.

Run !!!Boot to set up the PipeDream build environment.

Run !Amu.

Drag 'r32.Makefile' to !Amu... and wait...


Creating a package for distribution
----------------------------------- 

Make sure you've got the version number consistent! See version.c for notes.

Execute the Obey file r32.MakeRiscPkg.

This will copy only those files needed for distribution to PipeDream_4_xx_yy
in $.Temp and create the corresponding distributable ZIP package file.
