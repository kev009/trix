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


/*
 * Prototype TRIX ethernet driver.
 *
 * This driver is very primitive and implements only essential Ethernet
 * functionality. The responsibility for higher level protocol is left
 * to user protocol servers. This was done to enable many protocol to be
 * simultaneously supported.
 *
 * A user opens a port to the net specifying a protocol to be used. Only
 * one user port per protocol is allowed. This is assumed to be the server
 * for that type of packet protocol (Internet, Chaos, Trixgram).
 * In order to avoid gratutous copying of data, the packet is copied directly
 * from the user data window into the ethernet transmit buffer. This module
 * appends the ethernet header and check that the packet is a legal ethnet
 * packet but does not check the packet internals.
 *
 * Received packets are dispatched according to the protocol field in
 * the ethernet header. Again to avoid gratutous copying, there is no
 * kernel buffering. (Although this could change, it has). If there is no
 * pending read on the specified port or no port, the packet is discarded.
 * Otherwise the packet is copied into the user data window (minus the
 * ethernet header.
 * One possible exception to the above is that the kernel may handle
 * the internet address resolution protocol and keep the internet/ ethernet
 * translation table.
 *
 * This driver does not deal with name resolution at all. The dest address
 * is passed in via the write message.
 */

#include	"../h/param.h"
#include	"../h/args.h"
#include	"../h/calls.h"

#include	"net.h"
#include	"../sdu/iomsg.h"
#include	"../sdu/interrupt.h"


#define RETRYS 2			/* number of times to retransmit */

#define	PAMASK	0xf
#define JINTEN	0x10
#define TINTEN	0x20
#define AINTEN	0x40
#define BINTEN	0x80
#define RESET	0x100
#define RBBA	0x400
#define	AMSW	0x800
#define	JAM	0x1000
#define	TBSW	0x2000
#define	ABSW	0x4000
#define	BBSW	0x8000

#define TBUFSIZ	0x800			/* size of hardware transmit buffer*/

extern	short	mec[];


short	*mecstat = &mec[0];
short	*mecmebac = &mec[1];
short	*mecrom = &mec[0x200];
short	*mecram = &mec[0x300];
short	*mectrans = &mec[0x400];
short	*mecreca = &mec[0x800];
short	*mecrecb = &mec[0xC00];


extern long evec[];
extern struct conn *cn_find();

static	int	retrys;		/* for JAM processing */
static	int	jams;		/* number of jam interrupts */

/* stuff for ethernet name management */


short eth_me[3];			/* my ether address for now */

/* Accounting info */
int neterrs = 0;


eth_reset(types)
{
	static	int	savetypes;
	int	i;

	if(types >= 0)
		savetypes = types;

	*mecstat = RESET;
	retrys = 3;
	for(i = 0 ; i < 3 ; i++) {
		mecram[i] = mecrom[i];
		eth_me[i] = mecrom[i];
	}

	*mecstat |= AMSW;
	*mecstat |= PAMASK & savetypes;
	*mecstat |= (ABSW | BBSW);
	*mecstat |= (AINTEN | BINTEN);
}


eth_init(ether)
int	ether;
{
	/* here init interrupts */
	if(!ether) {
		/* NO ethernet board */
		eth_me[0] = 0x1234;
		eth_me[1] = 0x5678;
		eth_me[2] = 0x8ABC;
	}
	else
		eth_reset(0x6);
}


/* This should be the random library routine */

random()
{
	return(37);
}


eth_isme(eptr)
register short	*eptr;
{
	if(eptr[0] == eth_me[0] && eptr[1] == eth_me[1] && eptr[2] == eth_me[2])
		return(1);
	return(0);
}


eth_xlock()
{
	/* wait for transmitter */
	if(!(conns[0].cn_flags & CN_BUSY)) {
		conns[0].cn_flags |= CN_BUSY;
		return(1);
	}
	else
		return(0);
}


eth_xmt(addr, size, proto)
char	*addr;
{
	register struct	ethhdr	*ehdr;
	short	 offset;
	int	 s;
	int	 nsize;

	debug("xmt: size = %d, proto = %x\n", size, proto);

	if(size < EMINSIZ)
		nsize = EMINSIZ;
	else
		nsize = size;

	/* packet size has been checked earlier */
	
	if((offset = TBUFSIZ - nsize - sizeof(struct ethhdr)) <= 0)
		panic("Ethernet packet too large\n");
	
	ehdr = (struct ethhdr *)((char *)mectrans + offset);

	/* copy the data in from user space */
	if(kfetch(addr, (char *)ehdr, size) != size)
		return(0);

	/* destination os suppied by usr */
	for(s = 0 ; s < 3 ; s++)
		ehdr->eth_src[s] = eth_me[s];

	/* again ADDR_TYPES are special */
	if(ehdr->eth_proto != ADDR_TYPE)
		ehdr->eth_proto = (short)proto;

	if(*mecstat & TBSW)
		panic("Ether Xmit busy\n");

	/* Now send it */
	*mectrans = offset;			/* OK send it */
	*mecstat |= TBSW;
	*mecstat |= (TINTEN | JINTEN);		/* enable trans ints */

	debug("sent packet\n");
}


/* Dispatch to proper int service routine */

eth_int()
{
	short	stat = *mecstat;
	int	gotone = 0;

	if((stat & (ABSW | BBSW)) != (ABSW | BBSW)) {
		eth_rint();
		gotone = 1;
	}

	if(stat & JAM) {
		eth_jint();
		gotone = 1;
	}

	if(!(stat & TBSW)) {
		eth_xint();
		gotone = 1;
	}

	if(!gotone)
		printf("spurious ether int\n");
}


eth_xint()
{
	short	stat = *mecstat;

	if(!(stat & TINTEN))
		return;

	if(stat & TBSW) {
		printf("spurious xmt int\n");
		return;
	}

	if(!(conns[0].cn_flags & CN_BUSY)) {
		printf("int for xmt not busy\n");
	}

	debug("Got xmt interrupt\n");

	*mecstat &= ~(TINTEN | JINTEN | AINTEN | BINTEN);
	*mecstat |= (AINTEN | BINTEN);
	
	conns[0].cn_flags &= ~CN_BUSY;
	eth_wakeup(&conns[0],CN_XLOC);
}


eth_jint()
{
	register short	stat = *mecstat;

	jams++;

	if(!(stat & JAM) || !(stat & JINTEN))
		return;

	if(!(conns[0].cn_flags & CN_BUSY))
		printf("int for jam not busy\n");

	if(retrys--) {		/* try to retransmit */
		*mecmebac = -random();		/* get a random number */
		*mecstat |= JAM;		/* JAM is cleared by setting it!! */
		*mecstat &= ~(JINTEN | AINTEN | BINTEN);
		*mecstat |= (JINTEN | AINTEN | BINTEN);
	}
	else {				/* a serious failure-- reset */
		printf("trans retrys exceeded\n");
		eth_reset(-1);		/* reset the sucker */
		/* wakeup threads waiting on transmitter */
		if(conns[0].cn_flags & CN_BUSY) {
			conns[0].cn_flags &= ~CN_BUSY;
			eth_wakeup(&conns[0],CN_XLOC);
		}
	}
}


eth_rint()
{
	short	*rbp;			/* recieve buffer pointer */
	short	proto;
	short	which;			/* flag for which buffer */
	struct	packet	*pp;
	struct	conn	*cp;
	short	rhdr, stat = *mecstat;

debug("Got rcv interrupt\n");

	/* find find earliest received packet */
	switch(stat & (ABSW | BBSW)) {
	    case 0:				/* both full */
		if(stat & RBBA) {		/* B came first */
			rbp = mecrecb;
			which = BBSW;
		}
		else {
			rbp = mecreca;
			which = ABSW;
		}
		break;

	    case ABSW:			/* B full */
		rbp = mecrecb;
		which = BBSW;
		break;

	    case BBSW:
		rbp = mecreca;
		which = ABSW;
		break;

	    default:
		etherr("Strange call to eth_rint\n");
		return;
	}

	/* Process packet in hardware buffer */
	eth_rcv(rbp);

	/* reset receiver */
	*mecstat &= ~(AINTEN | BINTEN);
	if(*mecstat & TINTEN) {
		*mecstat &= ~TINTEN;
		which |= TINTEN;
	}
	*mecstat |= (which | AINTEN | BINTEN);
}


eth_rcv(rbp)
short	*rbp;			/* recieve buffer pointer */
{
	short	proto;
	struct	packet	*pp;
	struct	conn	*cp;
	short	rhdr, stat;

debug("In eth_rcv interrupt\n");

	/* discard error packets */
	rhdr = *rbp;
	if(rhdr & 0xA800) {		/* an error */
		debug("got an error packet\n");
		return;
	}
	rhdr &= 0x7ff;			/* offset of 1st free byte */
	rhdr -= 2;			/* actual data byte count */
	rbp += 1;		/* start of data (rbp is short *) */

	/* check size -- discard packets that are too big */
	if(rhdr > PKTSIZE) {
		etherr("Packet too big, discarding size = %d\n",rhdr);
		return;
	}

	/* Here do protocol dependant processing */

	/* ADDR_TYPE packets are treated specially */
	proto = ((struct ethhdr *)rbp)->eth_proto;
	if(proto == ADDR_TYPE)
		proto = ((struct ar_packet *)rbp)->ar_protocol;

	if((cp = cn_find(proto)) == (struct conn *)NULL) {
		etherr("No receiver for packet\n");
		return;
	}

	if((pp = pk_alloc()) == (struct packet *)NULL ) {
		etherr("No buffer for recieved packet\n");
		neterrs++;
		return;
	}

	/* copy data into receive buffer */
	mcopy(rbp, &pp->pk_ehdr, rhdr);
	pp->pk_dsize = rhdr;

	cn_enqueue(cp,pp);			/* put packet on queue */

	eth_wakeup(cp,CN_RLOC);
}


/* just print mesg for now */

etherr(mesg)
char	*mesg;
{
	debug("%s\n",mesg);
}
