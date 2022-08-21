@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.
@rem
@rem Copyright (C) 2013-2014 Stuart Swales
@rem
@rem ROOL repository
set ROOL_CVSROOT=:pserver:anonymous@riscosopen.org:/home/rool/cvsroot
@rem
echo Checking out RISC_OSLib from ROOL
cvs -d %ROOL_CVSROOT% -z9 co -D20130226 castle/RiscOS/Sources/Lib/RISC_OSLib/rlib/c
cvs -d %ROOL_CVSROOT% -z9 co -D20130226 castle/RiscOS/Sources/Lib/RISC_OSLib/rlib/h
cvs -d %ROOL_CVSROOT% -z9 co -D20130226 castle/RiscOS/Sources/Lib/RISC_OSLib/rlib/s
cvs -d %ROOL_CVSROOT% -z9 co -D20130226 castle/RiscOS/Sources/Lib/RISC_OSLib/rlib/VerIntern/h
cvs -d %ROOL_CVSROOT% -z9 co -D20130226 castle/RiscOS/Sources/Toolbox/Libs/flexlib/c
cvs -d %ROOL_CVSROOT% -z9 co -D20130226 castle/RiscOS/Sources/Toolbox/Libs/flexlib/h
