CC=clang
CFLAGS=-O2 -Wall -std=gnu11
OBJS = spec.o vict.o

default: spectest

%.o:%.c
	$(CC) -c -o $@ $< $(CFLAGS)

spectest: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: run clean
clean:
	rm -f spectest *.o

run: spectest
	./spectest

