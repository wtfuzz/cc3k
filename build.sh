#!/bin/sh

CFLAGS=-Iinclude

for i in src/*.c; do
	gcc $CFLAGS -c $i
done

gcc $CFLAGS -o cc3k_test cc3k_test.c *.o
