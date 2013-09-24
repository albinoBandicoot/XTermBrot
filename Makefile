CLANG=$(shell which clang)
ifeq ($(CLANG),)
	CC=gcc
else
	CC=clang
endif
all:	termctl.c mandelbrot.c
	$(CC) -o xtermbrot -g -pedantic -Wall -std=c99 termctl.c mandelbrot.c -I./ -lm
clean:
	rm -f xtermbrot
