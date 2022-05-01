# Makefile for Trix 1.0 Filesystem
CC = tcc

#CFLAGS=	-DKERNEL -DDEBUG
CFLAGS=		-DKERNEL

OBJS=		fsys.o inode.o file.o block.o l3.o soft.o

MODULES=	../low.o ../sdu.o \
		../kernel.a ../system.a ../kdev.a ../dev.a filesys.a ../lib.a

trix:		${MODULES}
		ld -T f00000 -e _start -x -o trix ${MODULES}
		cp trix /ftrix

update:		../filesys.a

../filesys.a:	${OBJS}
		make filesys.a
		-mv ../filesys.a ../filesys.A
		mv filesys.a ../filesys.a

filesys.a:	${OBJS} Makefile
		@rm -f filesys.a
		ar rc filesys.a ${OBJS}
		ranlib filesys.a

cache.o:	param.h fsys.h inode.h cache.h

smd.o:		param.h fsys.h inode.h cache.h
