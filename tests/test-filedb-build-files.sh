#!/bin/sh
#/----------------------------------------------------------------------\#
#| spkg - The Unofficial Slackware Linux Package Manager                |#
#|                                      designed by Ond�ej Jirman, 2005 |#
#|----------------------------------------------------------------------|#
#|          No copy/usage restrictions are imposed on anybody.          |#
#\----------------------------------------------------------------------/#

OPTS="-O2 -march=i486 -mtune=i686 -fomit-frame-pointer -D_GNU_SOURCE `pkg-config --cflags --libs glib-2.0`  -I.."
OPTS="-O2 -march=i486 -mcpu=i686 -fomit-frame-pointer -D_GNU_SOURCE `pkg-config --cflags --libs glib-2.0`  -I.."
#gcc -O0 -ggdb3 -o filedb filedb.c ../sys.c -D_GNU_SOURCE `pkg-config --cflags --libs glib-2.0` -I../ || exit

GCC=gcc-4.0
GCC=gcc-3.4.4
GCC=gcc

#$GCC -shared -o test-files.so files100k.c || exit
$GCC $OPTS -o test-bench-speed bench-speed.c ../filedb.c ../sys.c test-files.so || exit
$GCC $OPTS -o test-bench-size bench-size.c ../filedb.c ../sys.c test-files.so || exit
$GCC $OPTS -o test-bench-i486 bench-i486.c ../filedb.c ../sys.c test-files.so || exit

exit

for n in 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1000 10000 100000 1000000 ; do
  echo "** compiling $n..."
  gcc $OPTS "-DMAXHASH=($n)" -o test-bench-$n bench.c ../filedb.c ../sys.c test-files.so || exit
done

exit
for n in 4 8 16 32 64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1000 10000 100000 1000000 ; do
  echo "** benching $n..."
  ./test-bench-$n >> bench.out
done

#dot -Tpng -o index.png index.dot
#rm -f index.dot