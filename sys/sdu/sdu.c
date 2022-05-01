/*****************************************************************
 *                                                               *
 *                         Copyright (c) 1984                    *
 *               Massachusetts Institute of Technology           *
 *                                                               *
 * This material is a component of the TRIX system, developed by *
 * D. Goddeau, J. Sieber, and S. Ward of the                     *
 *                                                               *
 *                          RTS group                            *
 *               Laboratory for Computer Science                 *
 *            Massachusetts Institute of Technology              *
 *                Cambridge, Massachusetts 02139                 *
 *                                                               *
 * Permission to copy this software, to redistribute it, and to  *
 * use it for any purpose is granted, subject to the conditions  *
 * that (1) This copyright notice remain, unchanged and in full, *
 * on any copy or derivative of the software; and (2) that       *
 * references to or documentation of any copy or derivative of   *
 * this software acknowledge its source in accordance with the   *
 * usual standards in academic research.                         *
 *                                                               *
 * MIT has made no warrantee or representation that the          *
 * operation of this software will be error-free, and MIT is     *
 * under no obligation to provide any services, by way of        *
 * maintenance, update, or otherwise.                            *
 *                                                               *
 * In conjunction with products arising from the use of this     *
 * material, there shall be no use of the name of the            *
 * Massachusetts Institute of Technology nor of any adaptation   *
 * thereof in any advertising, promotional, or sales literature  *
 * without prior written consent from MIT in each case.          *
 *                                                               *
 *****************************************************************/


#include "../h/param.h"
#include "iomsg.h"
#include "mnc.h"
#include "bswap.h"
#include "../h/devmap.h"
#include "socket.h"
#include "../h/assert.h"
#include "interrupt.h"
#include <sys/timeb.h>

#define MAXWAIT		10000L	/* maximum loop cnt for busywait */

extern char mnccfg[];
extern char mncram[];
extern long mnc1map[];
extern long ashr[];
extern long ashx[];
extern long clk[];
extern long evec[];
extern long qtr[];
extern long smdintv[];

/*	extern long sdu[]; */

int timecnt;		/* count of busy-waiting timeouts */
int tascnt;		/* count of test and set timeouts */

struct iomsg iomsgs[NSOCKET];		/* actually lives on a buffer page */
int logsock = -1;
struct iomsg *logm;
char	msgbuf[32];
char	*msgbufp = msgbuf;	/* Next saved printf character */
char cb;			/* buffer outside stack for character */
union
{
	struct timeb timeb;
	char c[sizeof(struct timeb)];
} t;


intinit()
{
	sdugram(&ashr[0], IO_INTR, PCI_0_RINT);
	sdugram(&ashx[0], IO_INTR, PCI_0_TINT);
	sdugram(&ashr[1], IO_INTR, PCI_1_RINT);
	sdugram(&ashx[1], IO_INTR, PCI_1_TINT);
	sdugram(smdintv, IO_INTR, DISK_INT);
	sdugram(evec, IO_INTR, ETHER_INT);
	sdugram(qtr, IO_INTR, TAPE_RINT);
	sdugram(qtr, IO_INTR, TAPE_EINT);
	sdugram(clk, IO_INTR, CLK_INT);
	clkset();
}


/* set the clock from the battery backup */

clkset()
{
	struct iomsg *iom;
	int socket;

	socket = sduconn(0, 0, (int (*)())0);
	ASSERT(socket != -1);
	iom = &sockets[socket].iom;
	iom->fcode = IO_DATE;
	iom->buffer = sysaddr((caddr_t)t.c);
	sdusend(socket, 1);
	if(iom->value == 0) {	/* got clock ok, -1 is error code */
		extern	long	trixtime;
		lswap(&trixtime, &t.timeb.time);
/*		sswap(&dstflag, &t.timeb.dstflag);*/
/*		sswap(&timezone, &t.timeb.timezone);*/
	}
}


/* open a channel, send a single message */

sdugram(addr, funcode, int_line)
char *addr;
int funcode;
int int_line;
{
	struct iomsg *iom;
	int socket;

	socket = sduconn(0, 0, (int (*)())0);
	ASSERT(socket != -1);
	iom = &sockets[socket].iom;
	iom->fcode = funcode;		/* or msg */
	if (funcode == IO_INTR) {
		iom->intad = intaddr(addr);
		iom->vecnum = (long) int_line;
	}
	else
		iom->buffer = intaddr(addr);
	sdusend(socket, 1);
}


reboot()
{
	sdugram(0, IO_REBOOT, 0);
	for( ;; );
}


/* allocate and set up a socket */

sduconn(port, channel, handler)
int (*handler)();
{
	register struct socket *s;
	long tmpaddr;

	for(s = sockets ; s < &sockets[NSOCKET] ; s++)
		if(s->used == 0)
			goto found;
	return(-1);

found:
	s->iop = (struct ioport *)&mncram[devmap.port[port].ioport & PGOFSET];
	if(devmap.port[port].wakeup)
		s->iopwake = &mnccfg[devmap.port[port].wakeup & PGOFSET];
	else
		s->iopwake = 0;
	s->used = 1;
	s->handler = handler;
	s->iom.channel = channel;
	s->xmsg = &iomsgs[s - sockets];		/* get real msg block */
/*	tmpaddr = intaddr(&sdu[s - sockets]);	/* sdu interrupts me here */
/*	lswap(&s->wakeaddr, &tmpaddr);		/* save it byte rev'd */
	tmpaddr = sysaddr((caddr_t)s->xmsg);	/* get phys addr */
	lswap(&s->msgaddr, &tmpaddr);		/* save it for send */
	return(s - sockets);
}


/*
 *  send a message to the sdu
 *  Note that this guy can be called from interrupt level so do not sleep!
 */

sdusend(socket)
{
	int i;
	int priority;
	register struct ioport *iop;
	register struct socket *s;

	s = &sockets[socket];
	iop = s->iop;

	swapmsg(s->xmsg, &s->iom);	/* reverse bytes in msg */
	s->xmsg->done = 0;		/* flag as not done */

	priority = tas((caddr_t)&iop->busy, MAXWAIT);

	iop->msg = s->msgaddr;		/* install message address */
	iop->valid |= IOP_VALID;	/* mark port data as valid */

	i = MAXWAIT;
	while(s->xmsg->done == 0 && i-- > 0)
		delay();
	if(s->xmsg->done == 0)
		printf("sdu wait timeout\n");

	if (priority == -1)
		printf("tas failed\n");
}


/* reverse the bytes in all the fields of an iomsg */

swapmsg(x, y)
register struct iomsg *x, *y;
{
	sswap(&x->channel, &y->channel);
	sswap(&x->fcode, &y->fcode);
	lswap(&x->cnt, &y->cnt);
	lswap(&x->value, &y->value);
	sswap(&x->errcode, &y->errcode);
	lswap(&x->offset, &y->offset);
	lswap(&x->buffer, &y->buffer);
	lswap(&x->wakeup, &y->wakeup);
}


/* allocate multibus address space */

long sdumalloc(nuaddr, nbytes)
long nuaddr;
long nbytes;
{
	long val;
	struct iomsg *iom;
	int socket;

	socket = sduconn(0, 0, (int (*)())0);
	ASSERT(socket != -1);
	iom = &sockets[socket].iom;
	iom->fcode = IO_MALLOC;
	iom->buffer = nuaddr;
	iom->cnt = nbytes;	/* number of bytes to allocate */
	sdusend(socket, 1);
	swapmsg(iom, &iomsgs[socket]);	/* copy message back to him */
	val = iom->value;
	return(val);		/* return multibus address */
}


/* fill portions of sdu map */

sdumap(maddr, nuaddr, npages)
int maddr;			/* multibus address mapped to Nu space */
long nuaddr;			/* NuBus address to map to */
register npages;		/* number of pages in map to fill */
{
	long proto;		/* prototype map entry */
	extern long multi[];
	register long *map;

	if ((maddr >> 10) < 0) {
		printf("sdumap: maddr = 0x%x\n", maddr);
		printf("sdumap: maddr >> 10 = 0x%x\n", maddr >> 10);
	}

	debug("sdumap(%x,%x,%d)\n", maddr,nuaddr,npages);
	map = &multi[maddr >> 10];
	proto = (nuaddr >> 10) | MNC_MPEN;

	while(npages--) {
		lswap(map, &proto);	/* byte swap and fill entry */
		map++;
		proto++;
	}
}
