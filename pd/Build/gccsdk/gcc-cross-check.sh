#!/bin/sh

# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Copyright (C) 2014-2021 Stuart Swales

# set your cross environment up first
gcc -c ../../*.c ../../cmodules/*.c  -I../.. -I../../WimpLib/ -I ../../../../../coltsoft/trunk/cs-nonfree/Acorn/Library/32/CLib/msvchack  -funsigned-char -DCROSS_COMPILE -DHOST_GCCSDK -DRELEASED

