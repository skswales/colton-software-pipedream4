@rem This Source Code Form is subject to the terms of the Mozilla Public
@rem License, v. 2.0. If a copy of the MPL was not distributed with this
@rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

@rem Copyright (C) 2019-2021 Stuart Swales

@echo Checking out RISC_OSLib from ROOL

mkdir gitlab
mkdir gitlab\RiscOS
mkdir gitlab\RiscOS\Sources

pushd gitlab\RiscOS\Sources
git clone http://gitlab.riscosopen.org/RiscOS/Sources/Lib/RISC_OSLib.git Lib/RISC_OSLib
popd

pushd gitlab\RiscOS\Sources
git clone http://gitlab.riscosopen.org/RiscOS/Sources/Toolbox/Toolboxlib.git Toolbox/Toolboxlib
popd

pushd gitlab\RiscOS\Sources\Lib\RISC_OSLib
git checkout RISC_OSLib-5_82
popd

pushd gitlab\RiscOS\Sources\Toolbox\Toolboxlib
git checkout Libs-0_27
popd
