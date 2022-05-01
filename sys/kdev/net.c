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
 * This is the handler module for the ethernet driver.
 * This module assumes non-preemptive scheduling between threads of equal
 * priorities. If this should change, explicit interlocking will
 * be needed.
 */

#include "../h/param.h"
#include "../h/args.h"
#include "../h/calls.h"
#include "../h/status.h"
#include "../h/global.h"
#include "../h/link.h"
#include "../h/memory.h"
#include "../h/com.h"
#include "../h/thread.h"
#include "../h/domain.h"

#include "net.h"
/* Special cntrl op for jma -- see ../h/cntrl.h */

#define ETHERNET 1		/* yes there is an ethernet */

extern	int	neterrs;

extern	short	eth_me[];

struct	conn	conns[NCONNS];
struct	packet	packets[NPACKS];


/* Allocate a free structure insuring that no other connection has same protocol */

struct	conn	*
cn_alloc(proto)
{
	int	i;
	struct	conn	*cnp;
	struct	conn	*cnpfree = (struct conn *)NULL;

	for(i = 0, cnp = &conns[0] ; i<NCONNS ; i++, cnp++) {
		if(cnp->cn_flags == CN_FREE) {
			if(cnpfree == (struct conn *)NULL)
				cnpfree = cnp;
		}
		else if((cnp->cn_flags & CN_ALLOC) && (cnp->cn_proto == proto))
			return((struct conn *)NULL);
	}

	cnpfree->cn_proto = proto;
	cnpfree->cn_flags |= CN_ALLOC;

	debug("allocated connection %d\n",cnpfree - &conns[0]);
	return(cnpfree);
}


cn_free(cnp)
struct	conn	*cnp;
{
	cnp->cn_flags = CN_FREE;
}


struct	conn	*
cn_find(proto)
{
	int	i;

	for(i = 0 ; i < NCONNS ; i++) {
		if((conns[i].cn_flags & CN_ALLOC) && (conns[i].cn_proto == proto))
			 return(&conns[i]);
	}
	return((struct conn *)NULL);
}


/*
 * Put packet on receive queue for connection. This is called at high
 * processor priority at interrupt time.
 * Packets are added to the tail of the queue and removed form the beginning.
 */

cn_enqueue(cp,pkt)
struct	conn	*cp;
struct	packet	*pkt;
{
	pkt->pk_back = cp->cn_tail;
	pkt->pk_forw = (struct packet *)NULL;
	if(cp->cn_tail == (struct packet *)NULL) {
		/* queue is empty */
		cp->cn_tail = pkt;
		cp->cn_head = pkt;
	}
	else {
		cp->cn_tail->pk_forw = pkt;
		cp->cn_tail = pkt;
	}
	pkt->pk_flags |= PK_QUEUED;
}


/*
 * This routine removes packets from the head of th conn queue.
 * It is called from the user side at HIGH PRIORITY
 * during the critical sections. Again, this code assumes non preemptive
 * scheduling.
 */

struct packet *
cn_pktget(cp)
struct	conn	*cp;
{
	struct	packet	*tp;
	int	s;

	if(cp->cn_head == (struct packet *)NULL) 
		return((struct packet *)NULL);	/* queue is empty */
	tp = cp->cn_head;
	if(tp->pk_forw == (struct packet *)NULL) {
		cp->cn_head = (struct packet *)NULL;
		cp->cn_tail = (struct packet *)NULL;
	}
	else {
		tp->pk_forw->pk_back = (struct packet *)NULL;
		cp->cn_head = tp->pk_forw;
	}
	tp->pk_flags &= ~PK_QUEUED;
	return(tp);
}


struct	packet	*
pk_alloc()
{
	int	i;
	struct	packet	*pp;

	for(i = 0, pp = &packets[0] ; i < NPACKS ; i++, pp++) {
		if(pp->pk_flags == PK_FREE) {
			pp->pk_flags = PK_ALLOC;
			return(pp);
		}
	}
	return((struct packet *)NULL);
}


pk_free(pp)
struct	packet	*pp;
{
	pp->pk_flags = PK_FREE;
}


/* low level ethernet handler */

eth_handler(cp, mp)
register struct	message *mp;
struct	conn	*cp;
{
	struct packet *pkt;
	struct ethhdr ehdr;
	int s;
	char name[10];
	char *tp;
	short dest[3];

debug("eth_handler(%x) rerun = %x\n", mp->m_opcode, krerun());

	switch(mp->m_opcode) {
	    case LOOKUP:
		parsename(mp,name);
debug("proto = %s octal\n",name);
		s = 0;
		for(tp = name; *tp; tp++){
			s = (s << 3) + (*tp - '0');
		}
debug("proto = %x octal\n",s);
		if((mp->m_handle = eth_make(s)) != NULLHNDL){
			h_attach(mp->m_handle, T_CURRENT->t_subdom);
			mp->m_ercode = (OP_HANDLE | E_OK);
		}
		else mp->m_ercode = E_NOBJECT;
		return(0);

	    case WRITE:
		if(cp == (struct conn *)NULL){
			mp->m_ercode = E_HANDLE;
			printf("Bad handle\n");
			return(0);
		}
		if((mp->m_dwsize & 0x1) || (mp->m_dwsize < (sizeof(struct ethhdr))) ||
						(mp->m_dwsize > PKTSIZE)){
			mp->m_ercode = E_WINDOW;

			printf("Bad window size = %d\n",mp->m_dwsize);
			return(0);
		}
		kfetch(mp->m_dwaddr, &ehdr, sizeof(ehdr));

		/* OK, if the packet is for me, do the right thing */
		if(eth_isme(ehdr.eth_dest)){
			mp->m_param[0] = eth_tome(mp,cp->cn_proto);
			conns[0].cn_flags &= ~CN_BUSY;
			eth_wakeup(&conns[0],CN_XLOC);
			mp->m_ercode = E_OK;
			return(0);
		}
		if(ETHERNET == 0){
			mp->m_param[0] = 0;
			mp->m_ercode = E_OK;
			return(0);
		}	
		if(! eth_xlock(cp->cn_proto)){
			eth_sleep(&conns[0],CN_XLOC);
			return(1);
		}
		mp->m_param[0] = eth_xmt(mp->m_dwaddr, mp->m_dwsize, 
						cp->cn_proto);
		if(mp->m_param[0] <= 0) mp->m_ercode = E_FAILED;
			else mp->m_ercode = E_OK;
		return(0);

	    case READ:
		if(cp == (struct conn *)NULL){
			mp->m_ercode = E_HANDLE;
			return(0);
		}
		if(!eth_ispacket(cp)){
			eth_sleep(cp,CN_RLOC);
			return(1);
		}
		mp->m_param[0] = eth_read(cp, mp->m_dwaddr, mp->m_dwsize);
		if(mp->m_param[0] <= 0) mp->m_ercode = E_WINDOW;
			else mp->m_ercode = E_OK;
		return(0);

	    case CLOSE:
		if(cp != (struct conn *)NULL){
			while((pkt = cn_pktget(cp)) != (struct packet *)NULL){
				pk_free(pkt);
			}
			cn_free(cp);
		}
		mp->m_ercode = E_OK;
		return(0);

	    case GETSTAT:
		mp->m_ercode = eth_gstat(cp,mp);
		return(0);

	    default:
		mp->m_ercode = E_OPCODE;
		return(0);
	}
	return(0);
}


mec_make()
{
	return(p_kalloc(eth_handler, 0, 0));
}


/* Make ethernet handle */

hndl_t
eth_make(proto)
{
	static	int	etherfirst = 1;
	struct	conn	*cp;

	if(etherfirst) {
		eth_init(ETHERNET);
		etherfirst = 0;
	}

	if((cp = cn_alloc(proto)) == (struct conn *)NULL)
		return(NULLHNDL);
	else
		return(p_kalloc(eth_handler, cp, 0));
}


eth_gstat(cp,mp)
struct conn *cp;
struct message *mp;
{
	char name[PARSESIZE];
	unsigned char statbuf[256];
	char c;

	c = parsename(mp, name);
	if(c == -1)
		return(E_EINVAL);
	if(( c != 0) || (name[0] != 0))
		return(E_LOOKUP);

	c = t_FETCH(mp->m_dwaddr, statbuf, mp->m_dwsize);

	switch(STATLONG(&statbuf[1])) {
	    case STATUS_IPADDR:
		mcopy(eth_me, &statbuf[5], 6);
		break;
	    default:
		return(E_OPCODE);
	}
	t_STORE(mp->m_dwaddr, statbuf, mp->m_dwsize);
	return(E_OK);
}


eth_tome(mp,proto)
struct message *mp;
short proto;
{
	struct	packet	*pp;
	struct	conn	*cp;
	short	rhdr, stat;
	int	s;

	/* Here do protocol dependant processing */
	if((pp = pk_alloc()) == (struct packet *)NULL) {
		etherr("No buffer for recieved packet\n");
		neterrs++;
		return(0);
	}

	/* copy data into receive buffer */
	kfetch(mp->m_dwaddr, &pp->pk_ehdr, mp->m_dwsize);

	/* ADDR_TYPE packets are treated specially */
	if(proto != ADDR_TYPE)
		pp->pk_ehdr.eth_proto = proto;

	/* destination os suppied by usr */
	for(s = 0 ; s < 3 ; s++)
		pp->pk_ehdr.eth_src[s] = eth_me[s];

	if(proto == ADDR_TYPE)
		printf("arp packet in eth_tome\n");

	if((cp = cn_find(proto)) == (struct conn *)NULL) {
		etherr("No receiver for packet\n");
		pk_free(pp);
		return(0);
	}

	pp->pk_dsize = mp->m_dwsize;

	cn_enqueue(cp,pp);			/* put packet on queue */

	eth_wakeup(cp,CN_RLOC);
	return(mp->m_dwsize);
}


/*
 * If there is no packet ready, sleep on the connection. If there
 * is a packet, remove it from the queue and copy out data to user.
 * Remember only interrupt - user interlocking is done so use nonpreemptive
 * scheduling
 */

eth_ispacket(cp)
struct conn *cp;
{
	struct	packet	*tp;
	if(cp->cn_head == (struct packet *)NULL)
		return(0);
	else return(1);
}

eth_read(cp, dwind, dsize)
struct	conn	*cp;
char	*dwind;
{
	int	s;
	struct	packet	*tp;

	/* first wait for next packet */

	if((tp = cn_pktget(cp)) == (struct packet *)NULL){
		printf("No packet for eth read\n");
		return(-1);
	}

	if(dsize < tp->pk_dsize){
		pk_free(tp);
		return(-1);
	}

	s = t_STORE(dwind,&tp->pk_ehdr,tp->pk_dsize);
	debug("copied %d bytes from %x\n",s,tp->pk_data);	
	pk_free(tp);
	return(s);
}

eth_sleep(cp,flag)
struct	conn	*cp;
{
	cp->cn_flags |= flag;
	ksleep(0,cp);
}

eth_wakeup(cp, flag)
struct conn *cp;
{
	if((cp->cn_flags & flag) == 0) return;
	cp->cn_flags &= ~flag;
	kwakeup(0,cp);
}




