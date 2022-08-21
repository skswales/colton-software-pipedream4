@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2015-2020 Stuart Swales

chcp 1252

if exist local_env.bat call local_env

if exist "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86
if exist "%ProgramFiles%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat"      call "%ProgramFiles%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86

call pd_tbt

set PIPEDREAM_ROOT=N:\pipedream\%PIPEDREAM_TBT%

@rem display environment set for CROSS_COMPILE development
set

title %PIPEDREAM_ROOT%

cd /D %PIPEDREAM_ROOT%
