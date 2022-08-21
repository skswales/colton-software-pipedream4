#!/bin/sh

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Copyright (C) 2014-2018 Stuart Swales

clang \
 -target arm-none-eabi \
 -c ../../*.c ../../cmodules/*.c  \
 -I../.. -I../../WimpLib/ -I ../../../../../coltsoft/trunk/cs-nonfree/Acorn/Library/32/CLib/msvchack \
 -DCROSS_COMPILE -DHOST_CLANG -DRELEASED \
 -funsigned-char \
 -fno-builtin \
 -Wall -Wextra
