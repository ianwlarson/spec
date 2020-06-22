#!/bin/sh

gcc -c -o spec.o spec.c -O2 --std=gnu11
gcc -c -o vict.o vict.c -O2 --std=gnu11

gcc -o testo spec.o vict.o -O2 --std=gnu11
