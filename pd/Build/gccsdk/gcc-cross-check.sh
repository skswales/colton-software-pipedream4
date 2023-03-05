#!/bin/sh

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

# Copyright © 2014-2023 Stuart Swales

# set your cross environment up first
arm-none-eabi-gcc \
 -std=c99 \
 -I../.. \
 -I../../WimpLib/ \
 -I../../../../../coltsoft/trunk/cs-nonfree/Acorn/Library/32/CLib/clanghack \
 -I../../../../../coltsoft/trunk/cs-nonfree/Acorn/Library/32/CLib/msvchack \
 -I../../../../../coltsoft/trunk/cs-nonfree/Acorn/Library/32/CLib \
 -Os \
 -mfpu=vfp -mfloat-abi=hard \
 -funsigned-char \
 -fno-builtin \
 -DCROSS_COMPILE -DHOST_GCCSDK -DTARGET_RISCOS -DRELEASED \
 -c ../../*.c ../../cmodules/*.c ../../cmodules/riscos/*.c
