# Makefile for Trix 1.0 Kernel Devices
CC = tcc

CFLAGS=		-DKERNEL -DDEBUG

OBJS=		kdev.o clock.o sync.o pci.o peg.o smd.o cache.o net.o eth.o timer.o

MODULES=	../low.o ../sdu.o \
		../kernel.a ../system.a kdev.a ../dev.a ../filesys.a ../lib.a

trix:		${MODULES}
		ld -T f00000 -e _start -x -o trix ${MODULES}
		cp trix /dtrix

update:		../kdev.a

../kdev.a:	${OBJS}
		make kdev.a
		-mv ../kdev.a ../kdev.A
		mv kdev.a ../kdev.a

kdev.a:		${OBJS}
		rm -f kdev.a
		ar rc kdev.a ${OBJS}
		ranlib kdev.a
