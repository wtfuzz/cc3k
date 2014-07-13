#!/bin/sh

for i in src/*.c; do
	gcc -Iinclude -c $i
done
