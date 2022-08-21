# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Copyright (C) 2013-2016 Stuart Swales

# ROOL repository
ROOL_CVSROOT=:pserver:anonymous@riscosopen.org:/home/rool/cvsroot
#
echo Checking out RISC_OSLib from ROOL
cvs -d $ROOL_CVSROOT -z9 co -D20130226 castle/RiscOS/Sources/Lib/RISC_OSLib/rlib/c
cvs -d $ROOL_CVSROOT -z9 co -D20130226 castle/RiscOS/Sources/Lib/RISC_OSLib/rlib/h
cvs -d $ROOL_CVSROOT -z9 co -D20130226 castle/RiscOS/Sources/Lib/RISC_OSLib/rlib/s
cvs -d $ROOL_CVSROOT -z9 co -D20130226 castle/RiscOS/Sources/Lib/RISC_OSLib/rlib/VerIntern/h
cvs -d $ROOL_CVSROOT -z9 co -D20130226 castle/RiscOS/Sources/Toolbox/Libs/flexlib/c
cvs -d $ROOL_CVSROOT -z9 co -D20130226 castle/RiscOS/Sources/Toolbox/Libs/flexlib/h
