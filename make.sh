#/bin/sh

gcc -s -O3 -Wall -o make-gcc.exe make.c && ./make-gcc.exe $*
rm make-gcc.exe