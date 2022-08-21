#!/bin/sh
# set your cross environment up first
gcc -c ../../*.c ../../cmodules/*.c  -I../.. -I../../WimpLib/ -I ../../../../../coltsoft/trunk/cs-nonfree/Acorn/Library/32/CLib/msvchack  -funsigned-char -DCROSS_COMPILE -DHOST_GCCSDK -DRELEASED

