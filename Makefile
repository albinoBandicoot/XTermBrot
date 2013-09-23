CLANG=$(shell which clang)
ifeq ($(CLANG),)
	CC=gcc
else
	CC=clang
endif
all:	termctl.c mandelbrot.c
	$(CC) -O3 -o mandelbrot -g -pedantic -Wall -std=c99 termctl.c mandelbrot.c -I./ -lm
archive:
	tar -cf xtermbrot.tar mandelbrot.c termctl.c termctl.h Makefile
clean:
	rm -f mandelbrot
