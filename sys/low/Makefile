# Makefile for Trix 1.0 Kernel assembly support

CFLAGS=		-DKERNEL -DDEBUG

OBJS=		low.o

low.o:

update:		../low.o

../low.o:	low.o
		-mv ../low.o ../low.O
		cp low.o ../low.o
