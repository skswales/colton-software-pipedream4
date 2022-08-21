@rem pd\external\setup-windows.cmd

@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2013-2020 Stuart Swales

@rem Execute from %PIPEDREAM_ROOT%\pd\external directory

pushd %~dp0
IF NOT EXIST setup-windows.cmd EXIT

set COLTSOFT_CS_FREE=..\..\..\..\coltsoft\%PIPEDREAM_TBT%\cs-free
set COLTSOFT_CS_NONFREE=..\..\..\..\coltsoft\%PIPEDREAM_TBT%\cs-nonfree

mklink /j %CD%\cs-free    %COLTSOFT_CS_FREE%
mklink /j %CD%\cs-nonfree %COLTSOFT_CS_NONFREE%
@ rem Don't worry about those failing for Windows XP and VS2008

popd
