#!/bin/bash
INFILE=$1
tr -d '\r' < $INFILE | \
(LANG="C" sort -) | \
uniq | \
grep -v [[:digit:]] | \
grep -v "\." | \
grep -v "^'" | \
grep -v "'s"   \
 
