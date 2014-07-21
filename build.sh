#!/bin/sh

CFLAGS="$CFLAGS -Iinclude"

for i in src/*.c; do
	gcc $CFLAGS -c $i
done

ar -r libcc3k.a *.o
