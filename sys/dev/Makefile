# Makefile for Trix 1.0 Devices
CC = tcc

CFLAGS=		-DKERNEL -DDEBUG

OBJS=		video.o qtr.o

MODULES=	../low.o ../sdu.o \
		../kernel.a ../system.a ../kdev.a dev.a ../filesys.a ../lib.a

trix:		${MODULES}
		ld -T f00000 -e _start -x -o trix ${MODULES}
		cp trix /dtrix

update:		../dev.a

../dev.a:	${OBJS}
		make dev.a
		-mv ../dev.a ../dev.A
		mv dev.a ../dev.a

dev.a:		${OBJS}
		rm -f dev.a
		ar rc dev.a ${OBJS}
		ranlib dev.a
