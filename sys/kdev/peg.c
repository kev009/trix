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


#include	"../h/vcmem.h"
#include	"../h/param.h"
#include	"../h/calls.h"
#include	"../h/status.h"
#include	"../h/protocol.h"
#include	"peg.h"
#include	"key.h"


/*
 *  Revised to combine mouse and keyboard events into one
 *      stream of events.  Event structure (pegevent) is
 *	defined in mouse.h for now. ROB 8/7/84
 *
 *  Revised kernel key mappings for use with windows: SAW 5/19/84
 *   Following keys are interpreted specially at interrupt level:
 *	INS		call f4_function, if any.
 *	LINEFEED
 *	BRK/PAUS	if control & shift, reboot.
 *	PRNT		Debug printout control:
 *			UNSHIFTED: MORE control
 *			SHIFT: print NULLTHREAD
 *			CTRL: debug printout on/off
 *			CTRL-SHIFT:
 */

extern struct vcmem vccon;
extern long peg;		/* peg interrupt vector */

int	morewait;
int	(*f4_function)();


#define	SILOSIZE	128

struct	pegsilo {
	struct pegevent	*in;		/* current input position */
	struct pegevent	*out;		/* current output position */
	unsigned int	state;		/* state of the buffer */
	struct pegevent eventbuf[SILOSIZE]	/* actual event buffer */
}   pegsilo;		/* input buffer	*/


/* intialize keyboard state */

static
peginit()
{
	int	 i;
	register struct vcmem	*vp = &vccon;

	/* initialize silo */
	pegsilo.in = pegsilo.eventbuf;
	pegsilo.out = pegsilo.eventbuf;
	pegsilo.state = 0;

	/* note: order of cmd load is significant */
	vp->vc_acmd = VC_CRST;	/* reset */
	vp->vc_acmd = VC_R4;	/* set up SIO */
	vp->vc_acmd = VC_AWR4;
	vp->vc_acmd = VC_R5;
	vp->vc_acmd = VC_AWR5;
	vp->vc_acmd = VC_R3;
	vp->vc_acmd = VC_AWR3;
	vp->vc_acmd = VC_R1;
	vp->vc_acmd = VC_AWR1;	/* phew! */

	i = intaddr(&peg) | 1;
	i = ((i << 8) & 0xFF00FF00) | ((i >> 8) & 0x00FF00FF);
	i = ((i <<16) & 0xFFFF0000) | ((i >> 16) & 0xFFFF);

	vp->vc_intr = i;
	vp->vc_mcr &= ~VC_IE;
	vp->vc_freg0 |= VC_BDEN;

	/* set baud rate (some debate if needed) */
	vp->vc_scntl = (vp->vc_scntl & 0x0fff) | VCS_B1200;

	/* note: order of cmd load is significant */
	vp->vc_bcmd = VC_CRST;	/* reset */
	vp->vc_bcmd = VC_R4;
	vp->vc_bcmd = VC_BWR4;
	vp->vc_bcmd = VC_R5;
	vp->vc_bcmd = VC_BWR5;
	vp->vc_bcmd = VC_R3;
	vp->vc_bcmd = VC_BWR3;
	vp->vc_bcmd = VC_R1;
	vp->vc_bcmd = VC_BWR1;
}


static	long
peg_gstat(dev, type, dflt)
int	type;
long	dflt;
{
	extern	long	trixtime;

	switch(type) {
	    case STATUS_ATIME:
	    case STATUS_CTIME:
	    case STATUS_MTIME:
		return(trixtime);

	    case STATUS_IDENT:
		return(1);

	    case STATUS_PROT:
		return(PROT_STREAM);

	    case STATUS_ACCESS:
		return(0622);

	    case STATUS_OWNER:
		return(0);

	    case STATUS_SUSES:
		return(0);

	    case STATUS_ISIZE:
		return(0);

	    case STATUS_BANDW:
		return(9600);
	}

	return(dflt);
}


static	long
peg_pstat(dev, type, val)
int	 type;
long	 val;
{
	return(val);
}


peg_handler(passport, mp)
register struct	message *mp;
{
	switch(mp->m_opcode) {
	    case CONNECT:
		mp->m_ercode = E_ASIS;
		return(0);

	    case CLOSE:
		printf("peg_close\n");
		break;

	    case GETSTAT:
	    case PUTSTAT:
		mp->m_ercode = simplestat(mp, peg_gstat, peg_pstat, passport);
		return(0);

	    case READ:
	    {
		register int	size, count;
		register struct pegevent *oldout;
		register int	diff, events;

		/* wait for at least one event */
		if(pegsilo.in == pegsilo.out) {
			if(krerun() && kwakeval() == RECALL) {
				mp->m_param[0] = 0;
				break;
			}
			pegsilo.state |= PEGRLOC;
			ksleep(0, pegsilo.eventbuf);
			return(1);
		}

		if(mp->m_dwsize < sizeof(struct pegevent)) {
			 mp->m_ercode = E_SIZE;
			return(0);
		}

		/* find the actual number of bytes and events we are going to read */
		if(pegsilo.in > pegsilo.out) {
			count = (char *)pegsilo.in - (char *)pegsilo.out;
		}
		else {
			count = (char *)pegsilo.out - (char *)pegsilo.in;
			count = SILOSIZE * sizeof(struct pegevent) - count;
		}

		size = min(count, mp->m_dwsize); /* find max we can read for now */
		events = size / sizeof(struct pegevent); /* find num of events */
		size = events * sizeof(struct pegevent); /* read whole events */

		oldout = pegsilo.out;		/* stash an old copy of out ptr	*/
		/* how much till end */
		diff = (char *)&pegsilo.eventbuf[SILOSIZE] - (char *)oldout;

		/* now we find how we are going to split the t_STORE up depending */
		/* on whether the the events are contiguous in the buffer */

		if((pegsilo.in > oldout) || (diff >= size)) {
			kstore(mp->m_dwaddr, oldout, size);
			oldout += events;
			if(oldout >= &pegsilo.eventbuf[SILOSIZE])
				oldout = pegsilo.eventbuf;
			pegsilo.out = oldout;
		}
		else {
			kstore(mp->m_dwaddr, oldout, diff);
			mp->m_dwaddr += diff;
			diff = size - diff;
			kstore(mp->m_dwaddr, pegsilo.eventbuf, diff);
			pegsilo.out = pegsilo.eventbuf+(diff/sizeof(struct pegevent));
		}

		mp->m_param[0] = size;
		return(0);
	    }

	    default:
		mp->m_ercode = E_OPCODE;
		return(0);
	}

	mp->m_ercode = E_OK;
	return(0);
}


pegintr()
{
	/* there can be both a pending mouse and keyboard char */

	for( ; ; ) {
		if(keystat()) {
			key_int();	/* keyboard */
			continue;
		}
		if(msstat()) {
			ms_int();	/* mouse */
			continue;
		}

		/* all the interrupts are at least momentarily clear */
		return;
	}
}	


/* handle an interupt from the keyboard */

key_int()
{
	register int	c;
	static struct pegevent key_event = {0, 0, 0, 0};
	static int	shift, control = 0;
	char	*rcp;
	extern	Debug;
	extern	char	PEGWK;

	c = vccon.vc_adata & 0xFF;

	/* catch special keys */
	switch(c) {
	    case PRNT | 0x80:
		if(control) {
			if(!shift) {
				Debug = !Debug;
				printf("Debug is %s\n", Debug ? "ON" : "OFF");
			}
		}
		else {
			if(shift) {
				int	dbg = Debug;
				Debug = 1;
				t_print(NULLTHREAD);
				Debug = dbg;
			}
			else
				morewait = !morewait;
		}
	    case PRNT:
		return;

	    case INS | 0x80:
		if(f4_function) {
			int	dbg = Debug;
			Debug = 1;
			(*f4_function)();
			Debug = dbg;
		}		
	    case INS:
		return;

	    case BRKPS | 0x80:
		if(control && shift) {
			printf("sending reboot message\n");
			reboot();
		}
	    case BRKPS:
		return;

	    case SHIFT1 | 0x80:
		shift = DOWN;
		break;
	    case SHIFT1:
		shift = UP; 
		break;

	    case CNTL | 0x80:
		control = DOWN;
		break;
	    case CNTL:
		control = UP;
		break;
	}

	key_event.key = c;
	/* keep track of depressed keys */
	put_event(&key_event);
	/* arm low priority wakeup interrupt */
	PEGWK = 1;
}


pegwakeup()
{
	if((pegsilo.state & PEGRLOC) == 0)
		/* no one is sleeping */
		return;

	pegsilo.state &= ~PEGRLOC;
	kwakeup(0, pegsilo.eventbuf);
}


keystat()
{
	register struct vcmem *vp = &vccon;

	vp->vc_acmd = VC_R0;
	return(vp->vc_acmd & VC_R0RCA);	
}


msstat()
{
	register struct vcmem *vp = &vccon;

	vp->vc_bcmd = VC_R0;
	return(vp->vc_bcmd & VC_R0RCA);

}


/* read physical mouse device data and enter event in buffer */

ms_int()
{
	register struct vcmem *vp = &vccon;
	register int 	c = vp->vc_bdata;
	static	struct	pegevent ms_event = {0, 0, 0, 0};
	extern	char	PEGWK;
	static	int	count = 0;
	static	char	last_buttons;

	switch(count) {
	    case 0: 
		if((c & 0xf8) != 0x80)
			break;	/* garbage*/
		ms_event.buttons = c;
		count++;
		break;
	
	    case 1:
		ms_event.deltax += c;
		count++;
		break;

	    case 2: 
                ms_event.deltay += c;
		count++;
		break;

	    case 3: 
		ms_event.deltax += c;
		count++;
		break;

	    case 4: 
		ms_event.deltay += c;
		/*
		 *  if there is no substantial mouse motion we save what we have
		 *  to be summed into
		 */
		if((last_buttons != ms_event.buttons) || 
		   (ms_event.deltax > 1) || (ms_event.deltax < -1) ||
		   (ms_event.deltay > 1) || (ms_event.deltay < -1)) {
			put_event(&ms_event);
			PEGWK = 1;
			/* clear the residual mouse motion */
			ms_event.deltax = ms_event.deltay = 0;
		}
		last_buttons = ms_event.buttons;
		count = 0;
		break;
	}
}


/* make a handler for peg (combined mouse and keyboard) */

peg_make()
{
	peginit();
	return(p_kalloc(peg_handler, NULL, 0));
}


/*
 * Put event into event silo
 *
 * If it is a keyboard event, add it to buffer.
 * If it is a mouse event, and the the buttons have changed add it.
 * If it is a mouse event, and the buttons haven't changed, and there
 *    is an event in the buffer that hasn't been read, then change the
 *    event that still is in the buffer instead of adding a new one.
 */

put_event(event)
register struct pegevent	*event;
{
	register struct pegevent *ptr, *optr, *nptr;

	ptr = pegsilo.in;			/* pointer to current event */
	nptr = ptr + 1;				/* pointer to future event */
	if(nptr == &pegsilo.eventbuf[SILOSIZE])
		nptr = pegsilo.eventbuf;
	if(nptr == pegsilo.out)
		return (-1);	/* no space left in buffer */

	optr = ptr - 1;				/* pointer to past event */
	if(optr < pegsilo.eventbuf)
		optr = &pegsilo.eventbuf[SILOSIZE] - 1;

	if((event->key == 0) &&				/* is a mouse event */
	   (ptr != pegsilo.out) &&			/* is an old event in buf */
	   (optr->key == 0) &&				/* last was mouse */
	   (event->buttons == optr->buttons)) {		/* no butt change */
		/* modify old mouse event */
		optr->deltax += event->deltax;
		optr->deltay += event->deltay;
	}
	else {
		/* add new mouse event */
		ptr->key = event->key;
		ptr->buttons = event->buttons;
		ptr->deltax = event->deltax;
		ptr->deltay = event->deltay;
		pegsilo.in = nptr;
	}
}
