#!/bin/sh
clang \
 -target arm-none-eabi \
 -c ../../*.c ../../cmodules/*.c  \
 -I../.. -I../../WimpLib/ -I ../../../../../coltsoft/trunk/cs-nonfree/Acorn/Library/32/CLib/msvchack \
 -DCROSS_COMPILE -DHOST_CLANG -DRELEASED \
 -funsigned-char \
 -fno-builtin \
 -Wall -Wextra
