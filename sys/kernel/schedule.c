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


#include	"../h/param.h"
#include	"../h/calls.h"
#include	"../h/link.h"
#include	"../h/com.h"
#include	"../h/global.h"
#include	"../h/memory.h"
#include	"../h/domain.h"
#include	"../h/thread.h"
#include	"../h/assert.h"


domain_t	domains[];
thread_t	threads[];


struct	list	sleeplist;
struct	list	kcalllist;

static	thread_t *nextrun();


/*
 *  return pointer to next runable thread
 *  search is round-robin among domains, highest priority thread per domain.
 */

thread_t *
schedule()
{
	register domain_t  *ndp;
	register thread_t  *ntp;
	static	 domain_t  *sdp = &domains[0];
	int	 timeout;
	int	 s;

	s = hpl0();
	insched = 1;

	for(timeout = 0 ; ; timeout++) {
		dosched = 0;

		hplx(s);
		/* always look for kcalls first */
		{	struct	link	*l;

			if((l = l_first(&kcalllist)) != NULLLINK) {
				ntp = LINKREF(l, t_schdlnk, thread_t *);
				/* reschedule thread in real domains list */
				t_resched(ntp,
					&(ntp->t_subdom->d_domain->d_schdlst),
					ntp->t_priority);

				ASSERT(ntp->t_state & T_RUNABLE);
				goto runit;
			}
		}
		hpl0();

		/* then check system domain first */
		{
			if((ntp = nextrun(D_SYSTEM)) != NULLTHREAD)
				goto runit;
		}

		/* then do round robin from last schedule */
		for(ndp = sdp; ++ndp < &domains[DOMAINS]; )
			if((ntp = nextrun(ndp)) != NULLTHREAD)
				goto runit;
		for(ndp = &domains[0]; ndp <= sdp; ndp++)
			if((ntp = nextrun(ndp)) != NULLTHREAD)
				goto runit;

		if((timeout & 0xffff) == 0xffff)
			debug("looping in scheduler\n");
	}

runit:
	debug("scheduling T: %x\n", ntp);
	insched = 0;
	hplx(s);

	ntp->t_state |= T_HASRUN;
	return(ntp);
}


static	thread_t *
nextrun(dp)
register domain_t  *dp;
{
	register struct link  *l;
	register thread_t     *tp;

	if((dp->d_state & D_ALLOC) &&
	   ((l = l_first(&dp->d_schdlst)) != NULLLINK) &&
	   ((tp = LINKREF(l, t_schdlnk, thread_t *))->t_state & T_RUNABLE))
		return(tp);
	return(NULLTHREAD);
}


/* put the thread to sleep waiting for a wakeup on (sync,addr) */

I_sleep(sync, addr)
register caddr_t addr;
{
	register thread_t  *tp;
	int	 s;

	tp = T_CURRENT;
	debug("thread %x I_sleeping on %d:%x\n", tp, sync, addr);

	tp->t_wait.t_sleep.t_syncn = sync;
	tp->t_wait.t_sleep.t_saddr = addr;
	tp->t_state &= ~T_RUNABLE;
	tp->t_state |= T_SLEEPING;

	/* unschedule the thread leaving the domain active */
	t_unsched(tp);

	/* put thread in list of sleeping threads */
	l_link(&sleeplist, &tp->t_wait.t_sleep.t_slnk);

	/* turn on interrupts */
	s = spl0();

	/* wait for a wakeup */
	while(tp->t_state & T_SLEEPING) {
		/* this mechanism is used if on the user stack */
		debug("rescheduling\n");
		resched();
		/* this mechanism is used if on the kernel stack */
		/* runrun = 1; */
	}

	/* reset the priority (this is NOT done by resched()) */
	splx(s);
	return(tp->t_wait.t_wakeup);
}


I_wakeup(syncn, saddr)
caddr_t	saddr;
{
	return(kwakeup(syncn, saddr));
}
