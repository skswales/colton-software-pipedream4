#!/bin/sh

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Copyright (C) 2014-2021 Stuart Swales

clang \
 -target arm-none-eabi \
 -std=c99 \
 -I../.. \
 -I../../WimpLib/ \
 -I../../../../../coltsoft/trunk/cs-nonfree/Acorn/Library/32/CLib/clanghack \
 -I../../../../../coltsoft/trunk/cs-nonfree/Acorn/Library/32/CLib/msvchack \
 -I../../../../../coltsoft/trunk/cs-nonfree/Acorn/Library/32/CLib \
 -DCROSS_COMPILE -DHOST_CLANG -DTARGET_RISCOS -DRELEASED \
 -Os \
 -mfpu=vfp -mfloat-abi=hard \
 -funsigned-char \
 -fno-builtin \
 -Wall -Wextra -Wno-missing-field-initializers \
 -c ../../*.c ../../cmodules/*.c ../../cmodules/riscos/*.c
