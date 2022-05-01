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
 *  Driver for the Cipher Series 400 Quarterback Cartridge Tape Drive
 */
#include "../h/param.h"
#include "../h/args.h"
#include "../h/calls.h"
#include "../h/silo.h"
#include "../h/link.h"
#include "../kdev/cache.h"
#include "qtr.h"

#include "../sdu/iomsg.h"
#include "../sdu/interrupt.h"
#include "reg.h"

/* strange use of bno field for commands */
#define c_cmd c_bno

#define B_ERROR 0x1
#define QTRBUF 512 * 20

/* data associated with tape device */

struct	qtrstat {
	char	q_state;		/* current state of device */
	char	q_opencnt;		/* number of opens 1 or 0 */
	char	q_openf;		/* flag from open */
	char	q_alloc;		/* 1 => maps & intr handler setup */
	char	q_rdscnt;		/* index into status bytes */
	long	q_time;			/* time in QTICKS till next complete */
	char	*q_csr;			/* pointer to command/status register */
	char	*q_cmdr;		/* pointer to tape command and status reg */
	char	*q_dr;			/* pointer to data register */
	unsigned char	q_sts[NSTATS];	/* bytes from read status command */
	short	q_resid;		/* b_resid from last command */
	unsigned char	q_bot;		/* 1 => at beginning of tape */
	unsigned char	q_eof;		/* 1 => eof on last read, next read -> 0 */
}   qtrstat;

/* These use the cache structure so cmd requests can be queued in correct order */
struct cache qtrcmdbuf;		/* cmd buffers */
struct cache rqtrbuf;		/* buffer header used for raw io */
struct list qtrque;		/* use Jon's losing list stuff for now */
struct cache *qtractive;		/* command currnetly in progress */
char qtrdata[QTRBUF];		/* local buffer for tape data */

extern	char	qtr[];		/* interrupt vector */
extern	char	qtrizat[];	/* ptr to pte mapping in sdu registers */
extern	char	qmmizat[];	/* ptr to 2 ptes for data reg -> memory copy */


/* Exception handling */

struct	q_error_cond {
	unsigned short	exc_code;
	unsigned short	mask;
	char	 *errmsg;
}   qtremsg[] = {
	NOCAR,  MASK6,  "qtr: no cartridge present",
	NODRI,  MASK6,  "qtr: no drive present",
	WRIPRO, MASK3,  "qtr: cartridge is write-protected",
	EOM,	MASK0,  "qtr: end of media",
	RWABRT,	MASK1,  "qtr: read or write abort",
	RDBBX,  MASK1,  "qtr: read error, bad block xfer",
	RDFBX,  MASK1,  "qtr: read error, filler block xfer",
	RDND,   MASK3A, "qtr: read error, no data",
	RDEOM,  MASK1,  "qtr: read error, no data and EOM",
	FMRD,	MASK1,  "qtr: filemark read",
	ILCMD,  MASK5,  "qtr: illegal command",
	POR,	MASK5,  "qtr: power on/ reset",
	0,	0,	0
    };


qtr_handler(passport,mp)
struct message *mp;
{
	int ntodo, offset,count,err;
	switch(mp->m_opcode){
		case CLOSE:
/* printf("qtr:CLOSE\n"); */
			qtr_close(passport);
			mp->m_ercode = E_OK;
			break;
		
		case READ:
/* printf("qtr:READ\n"); */
			ntodo = mp->m_dwsize;
			if((ntodo % TPBLKSIZ) != 0){
				mp->m_ercode = E_SIZE;
				break;
			}
			if(qtrstat.q_eof){
/*				qtrstat.q_eof = 0; */
				mp->m_param[0] = 0;
				mp->m_ercode = E_OK;
				break;
			}
			offset = 0;
			while(ntodo > 0){
				if(ntodo > QTRBUF)  count= QTRBUF;
					else count = ntodo;
				rqtrbuf.c_data = qtrdata;
				rqtrbuf.c_length = count;
				err = qtr_rdwr(READ);
				ntodo -= (count - qtrstat.q_resid);
				t_STORE(mp->m_dwaddr + offset, qtrdata,
							count - qtrstat.q_resid);
				if(qtrstat.q_eof) break;
				if(err & C_ERROR) break;
				offset += (count - qtrstat.q_resid);
/* printf("resid = %d\n",qtrstat.q_resid); */
			}
			mp->m_param[0] = mp->m_dwsize - ntodo;
			mp->m_ercode = E_OK;
			break;
		case WRITE:
			ntodo = mp->m_dwsize;
			if((ntodo % TPBLKSIZ) != 0){
				mp->m_ercode = E_SIZE;
				break;
			}
			offset = 0;
			while(ntodo > 0){
				if(ntodo > QTRBUF)  count= QTRBUF;
					else count = ntodo;
				rqtrbuf.c_data = qtrdata;
				rqtrbuf.c_length = count;
				t_FETCH(mp->m_dwaddr + offset, qtrdata, count);
				err = qtr_rdwr(WRITE);
				ntodo -= (count - qtrstat.q_resid);
				if(err & C_ERROR) break;
				offset += (count - qtrstat.q_resid);
			}
			mp->m_param[0] = mp->m_dwsize - ntodo;
			mp->m_ercode = E_OK;
			break;

		default:
			mp->m_ercode = E_OPCODE;
			break;
	}
	return(mp->m_ercode);
}


qtr_con_handler(passport,mp)
struct message *mp;
{
	switch(mp->m_opcode){
		case CONNECT:
			if(qtr_open(passport,mp) == E_OK){
				mp->m_handle = t_MAKEPORT(qtr_handler,passport,0);
				mp->m_ercode = E_OK | OP_HANDLE;
				break;
			}
			else{
				mp->m_ercode = E_CONNECT;
				break;
			}

		default:
			mp->m_ercode = E_OPCODE;
			break;
	}
	return(mp->m_ercode);
}

qtr_make(dev){
	qtractive = NULLCACHE;

	if(!qtrstat.q_alloc) {
		/* setup pointers to maps and tell sdu where to forward interrupts */
		qtrstat.q_csr = (char *)&qtrizat[CSR1];
		qtrstat.q_cmdr = (char *)&qtrizat[TP_CMD];
		qtrstat.q_dr = (char *)&qtrizat[TP_DATA];
		qtrstat.q_alloc = 1;
		rqtrbuf.c_data  =  qtrdata;
		rqtrbuf.c_state = 0;
		rqtrbuf.c_cmd = 0;
	}

	qtrcmd(Q_NULL_CMD,1);
	qtrcmd(Q_RESET,1);
	return(t_MAKEPORT(qtr_con_handler, dev, 0));
}

/*
 *  establish the control/status, command, and data registers,
 *  sets up handlers for interrupts, and positions the tape.
 */

qtr_open(flag,mp)
unsigned int flag;
struct message *mp;
{
	if(qtrstat.q_opencnt) {
		return(E_CONNECT);
	}

	/* Wait for a possible rewind to finish by synchronizing on qtrcmdbuf */
	qtrcmd(Q_NULL_CMD, 1);

	qtrstat.q_opencnt = 1;		/* miscellaneous initializations */
	qtrstat.q_openf = flag;
	qtractive = NULLCACHE;

	if(!qtrstat.q_alloc) {
		/* setup pointers to maps and tell sdu where to forward interrupts */
		qtrstat.q_csr = (char *)&qtrizat[CSR1];
		qtrstat.q_cmdr = (char *)&qtrizat[TP_CMD];
		qtrstat.q_dr = (char *)&qtrizat[TP_DATA];
		qtrstat.q_alloc = 1;
		rqtrbuf.c_data  =  qtrdata;
		rqtrbuf.c_state = 0;
		rqtrbuf.c_cmd = 0;
	}

	if((*qtrstat.q_csr & TP_ONLINE) == 0) {
		/* initialize formatter and bot/eof flags */
		if(qtrcmd(Q_RESET, 1) & B_ERROR || qtrcmd(Q_SELECT0, 1) & B_ERROR) {
			qtrstat.q_opencnt = 0;
			return(E_CONNECT);
		}
		qtrstat.q_bot = 1;
	}
	return(E_OK);
}


/* reads a TPBLK of data from or to qtrdata */
qtr_rdwr(rdwr){

	register struct cache *cp = &rqtrbuf;
	int val;
	cp->c_state &= ~C_ERROR;
	if(cp->c_state & C_USBSY){
		cp->c_state |= C_UWAIT;
		I_sleep(0, cp);
		
	}
	cp->c_state |= C_USBSY;
	IOWait(cp);

	if(rdwr == WRITE)
		cp->c_state |= C_WRBSY;
	else
		cp->c_state |= C_RDBSY;

	val = qtr_queue(cp);
	IOWait(cp);
	return(cp->c_state);
}

/* write filemark and take offline if necessary */

qtr_close(dev)
{
	/* going offline from a write will write a filemark */
	qtrcmd(Q_GO_OFFLINE, 0);
	qtrstat.q_opencnt = 0;
}


/*
 *	qtr_queue   - Verifies the correctness of the incoming request,
 *		      places it on the queue, and gives the formatter a kick.
 */

qtr_queue(cp)
register struct cache *cp;
{
	int s;
	register struct qtrstat *q;
	struct list *l;

	if(cp == NULLCACHE)
		return;

	if(!CMDP(cp)) {
		/* must be a read or a write */
		if((cp->c_length % TPBLKSIZ) != 0) {
			/* must read/write multiples of TPBLKSIZ bytes */
			printf("qtr: bad count %D to strategy\n", cp->c_length);
			/* We ought to have some error code in cache struct */
			freebfr(cp);	/* give buffer back */
			return(-1);
		}
		/* filemark on last read?  If so, this guy gets an eof */
		q = &qtrstat;
		if((cp->c_state & C_RDBSY) && (q->q_eof)) {
			/* we must indicate ) length read somehow */
			freebfr(cp);
			q->q_eof = 0;
			return(0);
		}
	}

	/* here queue request, check to make sure it isn't already queued */
	if(cp->c_next.next != NULLLINK) printf("WARNING: Cache queue error\n");

	s = splx(SPL_DISK);			/* protect queue */
	for(l = &qtrque; l->next != NULLLINK; l = (struct list *)(l->next)); 
	l_link(l, &cp->c_next);
	splx(s);

	qtr_start();
	return(1);
}


/* qtr_start take the next item off of the queue and start it */
qtr_start(){
	register struct cache	*cp;
	struct link *l;
	int s;

    /*
     *  Verify that there is indeed a request and then mark the
     *  disk as active.
     */

	s = splx(SPL_DISK);
	if(qtractive != NULLCACHE){
		splx(s);
	    	return;
	}
	if((l = qtrque.next) == NULLLINK) cp = NULLCACHE;
	else{
		l_unlink(l);
		l->next = NULLLINK;
		l->prev = NULLLINK;
		cp = LINKREF(l, c_next, struct cache *);
	}


	if((cp == NULLCACHE) || (!(cp->c_state & C_IOBSY))) { 
		splx(s);
		return;
	}

	qtractive = cp;

	qtrstate();		/* start the formatter */
	splx(s);

}

/*
 *	qtrstate -  Look at current state and current buffer to determine
 *		    what to do next, then twiddle the appropriate control
 *		    lines (gee, a controller would be nice).
 *		    Always called at SPL_DISK
 */

qtrstate()
{
	int	unit;
	int	i;
	char	flags;
	register struct	cache *cp;
	char	csr;

	if((cp = qtractive) == NULLCACHE)
		return;

	qtrstat.q_eof = 0;
/* printf("state = %x cmd = %x\n",qtrstat.q_state,cp->c_cmd); */
	switch(qtrstat.q_state) {
	    case IDLE:
		/* issue a command to the formatter */
		if(CMDP(cp)) {
			/* not read/write */
			switch(cp->c_cmd) {
			    case Q_RESET:
				flags = *qtrstat.q_csr;
				*qtrstat.q_csr = TP_RESET;
				if(flags & TP_EX) {
					/* we won't get EXC interrupt, so fake it */
					qtrstat.q_state = EXCEPT;
					qtrstate();	/* recursive! */
				}
				else {
					/* we should get an EXC interrupt */
					qtrtset(cp->c_disk, TRESET);
				}
				return;
			case Q_GO_OFFLINE:
				/* take offline, will get RDY when rewind finishes */
				*qtrstat.q_csr = 0;
				if(qtrstat.q_bot) {
					/* no RDY intr when at bot, free buffer now */
					freebfr(cp);
				}
				else {
					qtrtset(cp->c_disk, TREWIND);
				}
				return;
			case Q_WTMARK:
			case Q_RDMARK:
				/* these commands require that the tape be online */
				if((*qtrstat.q_csr & TP_ONLINE) == 0)
					*qtrstat.q_csr = TP_ONLINE;
				/* fall thru */
			default:
				/*
				 *  Just put command on data bus.
				 *  Cmd will move tape (we shouldn't be here with
				 *  an RDSTAT cmd), so update q_bot.
				 */
				qtrstat.q_bot = 0;
				*qtrstat.q_cmdr = cp->c_cmd;
				qtrtset(cp->c_disk, TCMDACPT);
				break;
			}
		}
		else
		{
			/* Read/write.  Make sure tape is online and issue command */
			if((*qtrstat.q_csr & TP_ONLINE) == 0) {
				*qtrstat.q_csr = TP_ONLINE;
			}
			*qtrstat.q_cmdr = (cp->c_state & C_RDBSY) ? Q_READ : Q_WRITE;
			qtrtset(cp->c_disk, TIO);
		}
		/* Set REQ, will get RDY interrupt when cmd is accepted */
		csr = *qtrstat.q_csr;
		csr &= TP_ONLINE;
		csr |= TP_REQ;
		*qtrstat.q_csr = csr;
		return;

	case CMDACPT:
	case RDSACPT:
		/*
		*  Normal or read status cmd accepted.  Reset REQUEST to
		*  begin command execution.  Will get RDY interrupt next.
		*/
		csr = *qtrstat.q_csr;
		csr &= TP_ONLINE;
		*qtrstat.q_csr = csr;
		qtrtset(cp->c_disk, TCMD);
		return;

	case RDSTAT:
		/*
		*  Acknowledge status byte by toggling REQUEST, which
		*  causes next byte (if present) to be sent.  Will get
		*  RDY interrupt next.
		*/
		csr = *qtrstat.q_csr;
		csr &= TP_ONLINE;
		csr |= TP_REQ;
		*qtrstat.q_csr = csr;
		for (i = 0; i < 20 ; i++)	 /* strobe for > 10 && < 500 usec */
			/* null */;
		csr = *qtrstat.q_csr;
		csr &= TP_ONLINE;
		*qtrstat.q_csr = csr;
		qtrtset(cp->c_disk, TRDSTAT);
		return;		

	case RDSDONE:
		/*
		*  Shouldn't get here in this state, so let's just
		*  wait for our RDY interrupt.
		*/
		return;

	case EXCEPT:
		/*
		*  Exception interrupt, issue a read status command.
		*  In the case of a RESET cmd, we must disable Reset-bar.
		*  Will get RDY interrupt when cmd is accepted.
		*/

		*qtrstat.q_cmdr = Q_STATUS;
		csr = *qtrstat.q_csr;
		csr &= TP_ONLINE;
		csr |= TP_REQ;
		if (CMDP(cp) && (cp->c_cmd == Q_RESET))
			csr &= ~TP_RESET;
		*qtrstat.q_csr = csr;
		qtrtset(cp->c_disk, TCMDACPT);
		return;		

	case XREAD:
	case XWRITE:
		/*
		*  Data block ready (read) or ready for next data 
		*  block (write).  Transfer data between memory and
		*  data register.  Note that buffer is freed as soon
		*  as transfer is done.
		*/
		csr = *qtrstat.q_csr;
		csr &= TP_ONLINE;
		*qtrstat.q_csr = csr;
		if(*qtrstat.q_csr & TP_RDY)
			qtrblt(cp);
		return;

	default:
		/* Say what ? */
		printf("qtr: unexpected state %d in start\n", qtrstat.q_state);
		return;
	}
}


/*
 *	qtrintr - Entry point for RDY and EXC interrupts.  Make a state
 *		  transition and possibly get status bytes.  If we are
 *		  done with the buffer, free it.
 */

qtr_int()
{
	int unit;
	register struct cache *cp;

	if(qtractive == NULLCACHE){
/*		printf("Unexpectaed qtr interrupt\n"); */
	}

	cp = qtractive;
	if(*qtrstat.q_csr & TP_EX) {
		/* exception interrupt, try to initiate reading status bytes */
		switch(qtrstat.q_state) {
		    case RDSACPT:
		    case RDSTAT:
			/* Whoa, let's just call it quits and free buffer */
			if(cp != NULLCACHE)
			/* We need an error code in the cache */
			printf("qtr: exception processing exception\n");
			qtrstat.q_state = IDLE;
			freebfr(cp);
			return;
		    default:
			/* Ok, let's get them bytes */
			qtrstat.q_state = EXCEPT;
			qtrstate();
			return;
		}
	}

	if((*qtrstat.q_csr & TP_RDY) == 0) {
		/* haha, the unknown interrupt strikes again!! */
		return;
	}

	switch(qtrstat.q_state) {
	    case EXCEPT:
		/* Read status cmd accepted, initiate byte transfer */
		qtrstat.q_state = RDSACPT;
		qtrstate();
		return;

	    case RDSACPT:
		/* First status byte ready, keep going */
		qtrstat.q_state = RDSTAT;
		qtrstat.q_rdscnt = 0;
		qtrstat.q_sts[qtrstat.q_rdscnt++] = *qtrstat.q_cmdr;
		qtrstate();
		return;

	case RDSTAT:
		/*
		*  Grab and acknowledge status byte.
		*/
		qtrstat.q_sts[qtrstat.q_rdscnt++] = *qtrstat.q_cmdr;
		qtrstate();
		if (qtrstat.q_rdscnt >= NSTATS)
		{
			/*
			*  Got em all, wait for mysterious RDY intr.
			*/
			qtrstat.q_state = RDSDONE;
			if (qtrerr(cp))
			{
				cp->c_state |= C_ERROR;
				qtrstat.q_resid = cp->c_length;
			}
		}
		return;

	case RDSDONE:
		/*
		*  All done, go IDLE and free buffer.
		*/
		qtrstat.q_state = IDLE;
		freebfr(cp);
		return;

	case CMDACPT:
		if (CMDP(cp))
		{
			/*
			*  All done, go IDLE and free buffer.
			*  Update bot flag if cmd positioned tape.
			*/
			qtrstat.q_state = IDLE;
			if ((cp->c_cmd & POSMASK) == POSITION)
				qtrstat.q_bot = 1;
			freebfr(cp);
			return;
		}
		/*
		*  Read/write accepted, start data transfer.
		*/
		else if (cp->c_state & C_RDBSY)
			qtrstat.q_state = XREAD;
		else
			qtrstat.q_state = XWRITE;
		qtrstate();
		return;

	case XREAD:
	case XWRITE:
		/*
		*  Data block ready (read) or ready for data block (write).
		*/
		if (cp->c_length == 0)
		{
			freebfr(cp);
		}
		else
			qtrstate();
		return;

	case IDLE:
		/*
		*  Formatter has accepted command or rewind is done.
		*/
		if (CMDP(cp) && (cp->c_cmd == Q_GO_OFFLINE))
		{
			/*
			*  Stay IDLE, free buffer.
			*/
			qtrstat.q_bot = 1;
			freebfr(cp);
			return;
		}
		else
		{
			/*
			*  Start the command.
			*/
			qtrstat.q_state = CMDACPT;
			qtrstate();
			return;
		}

	default:
		/*
		*  Say what ?
		*/
		printf("qtr: unexpected state %d in intr\n", qtrstat.q_state);
		qtrstat.q_state = IDLE;
		freebfr(cp);
		return;
	}
}


/*
*  Free buffer header and start action on next request.
*/

freebfr(cp)
struct cache *cp;
{
	struct qtrstat *q;

	q = &qtrstat;
	if (cp == NULLCACHE)
		return;
	qtrstat.q_resid = cp->c_length;

	IODone(cp);

	cp->c_state &= ~C_USBSY;
	if(cp->c_state & C_UWAIT){
		cp->c_state &= ~C_UWAIT;
		I_wakeup(0,cp);
	}
qtractive = NULLCACHE;
	qtr_start();
}


/* execute a tape command, possibly waiting until it finishes */

qtrcmd(cmd, count)
unsigned int	cmd;
int	 count;
{
	register struct cache *cp;
	int s;

	cp = &qtrcmdbuf;
	qtrstat.q_state = IDLE;
	s = splx(SPL_DISK);
	IOWait(cp);

	if(cmd == Q_NULL_CMD) {
		/* qtropen was using the buffer to synchronize rewind completion */
		splx(s);
		qtrstat.q_resid = cp->c_length;
		IODone(cp);
		return(0);
	}

	cp->c_state = (C_USBSY | C_RDBSY);
	splx(s);
/*	bp->b_dev = dev;
	bp->b_repcnt = count;
*/	cp->c_cmd = cmd;

	qtr_queue(cp);

	if(cmd == Q_GO_OFFLINE) {
		/*
		 *  don't wait for the rewind
		 *  qtrintr will clear B_BUSY and do iodone(bp) when rewind finishes
		 */
		return(0);
	}

	IOWait(cp);
	if(cp->c_state & C_USBSY) printf("qtrcmdbuf still USBSY after command\n");
/* printf("command completed %x\n",cp->c_cmd); */
	return(0);
}


/*
 *  ioctl calls for retension, erase tape, read file mark, and bot commands
 *  also implement a get status ioctl call
 */


/*
 *  Actually do a data transfer, memory mapping for raw transfers.
 *  Note that bus error panics are disabled during the transfer in
 *  order to prevent a crash if there isn't any data on the tape.
 */

qtrblt(cp)
register struct cache *cp;
{
	caddr_t addr, paddr;
	register char *to, *from;
	register nbytes = TPBLKSIZ;
	struct qtrstat *q;
	char csr0;

	addr = (caddr_t)cp->c_data;

	/* get to/from pointers going the right way */
	if(cp->c_state & C_RDBSY) {
		to = addr;
		from = qtrstat.q_dr;
	}
	else {
		to = qtrstat.q_dr;
		from = addr;
	}

	/*
	*  Move bytes between memory and data register.  Bus error panics
	*  are disabled because a bus timeout may occur if there isn't
	*  any data during a read (the retry is slow, see p.38).  
	*/
	bus_panic(0);
	do 
		*to++ = *from++;
	while(--nbytes);
	bus_panic(1);

	cp->c_length -= TPBLKSIZ; 
	cp->c_data += TPBLKSIZ;

}


/* timeout interrupt handler */

qtrtime()
{
	int s;
	struct cache *cp;

	if(--qtrstat.q_time <= 0) {
		s = splx(SPL_DISK);
		if((cp = qtractive) != NULLCACHE) {
			if(CMDP(cp) && (cp->c_cmd == Q_RESET)) {
				printf("qtr: timeout on RESET\n");
				cp->c_state |= C_ERROR;
				freebfr(cp);
			}
			else {
				printf("qtr: lost interrupt\n");
				qtr_int();
			}
		}
		(void) splx(s);
	}
	else{
		clk_call(qtrtime, 0, QTRTICK);	 
	}
}


/* establish a timeout and set pending ticks */

qtrtset(addr,n)
char *addr;
{
	if(qtrstat.q_time == 0)
		clk_call(qtrtime, 0, QTRTICK);
	qtrstat.q_time += n;
}


/*
 *  check for error after rdstatus and print error message
 *  return a 1 if there really was an error
 */

qtrerr(cp)
struct cache *cp;
{
	int i;
	int found = 0;
	unsigned short summary, retries;
	struct qtrstat *q;
	unsigned char csr;

	if(cp == NULLCACHE) {
		printf("qtr: qtrerr bp == NULL\n");
		return(0);
	}
	/* Get summary bytes */
	summary = (qtrstat.q_sts[0] << 8) | qtrstat.q_sts[1];
	retries = (qtrstat.q_sts[2] << 8) | qtrstat.q_sts[3];

	/*
	*  Check for "errors" which are really ok.  These are:
	*	power on/reset if we issued a RESET
	*	filemark read if we issued a READ or a RDMARK
	*/
 	if(CMDP(cp) && (cp->c_cmd == Q_RESET) && 
	   ((summary & MASK5) == POR))
		return(0);
	if(!CMDP(cp) && (cp->c_state & C_RDBSY) &&
	   ((summary & MASK1) == FMRD)) {
		qtrstat.q_eof++;
		return(0);
	}
	if(CMDP(cp) && (cp->c_cmd == Q_RDMARK) &&
	   ((summary & MASK1) == FMRD))
		return(0);

	/* find and print error message */
	for(i = 0; qtremsg[i].errmsg; i++) {
		if ((unsigned short)(summary & qtremsg[i].mask) 
			== qtremsg[i].exc_code)
		{
			printf("%s\n", qtremsg[i].errmsg);
			found = 1;
		}
	}
	if (!found) printf("qtr: unknown error = 0x%x\n", summary);
	cp->c_state |= C_ERROR;
	return(1);
}


bus_panic(){
}

