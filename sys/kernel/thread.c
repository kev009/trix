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
#include	"../h/args.h"
#include	"../h/assert.h"


thread_t  threads[THREADS+1];
thread_t  *T_CURRENT = &threads[0];	/* we need to start in some thread */

struct	list	sleeplist;
struct	list	kcalllist;


t_init()
{
}


t_print(tp)
register thread_t  *tp;
{
	register struct	link	*l;

	if(tp == NULLTHREAD) {
		for(tp = &threads[0] ; tp < &threads[THREADS] ; tp++)
			/* print each thread stack */
			if(tp->t_state!=T_FREE && !(tp->t_state&T_REQUEST)) {
				register thread_t  *ttp = tp;
				/* recurse up the thread stack */
/*				do {
					t_print(ttp);
				} while((ttp = ttp->t_stack) != NULLTHREAD);
*/
				t_print(ttp);
			}
		debug("sleeplist: ");
		FOREACHLINK(l, sleeplist) {
			tp = LINKREF(l, t_schdlnk, thread_t *);
			debug("%x ", tp);
		}
		debug("\n");
		return;
	}

	if(tp->t_state != T_FREE) {
		if(tp == T_CURRENT)
			debug("*");
		debug("T(%x): pri=%d  state=%x=", tp, tp->t_priority, tp->t_state);
		if(tp->t_state & T_RUNNING)
			debug("RUNNING ");
		if(tp->t_state & T_RUNABLE)
			debug("RUNABLE ");
 		if(tp->t_state & T_SLEEPING)
			debug("SLEEP(%x,%d) ", tp->t_wait.t_sleep.t_saddr,
					    tp->t_wait.t_sleep.t_syncn);
		if(tp->t_state & T_REQUEST)
			debug("REQUEST ");
		if(tp->t_state & T_CLOSE)
			debug("CLOSE ");
		if(tp->t_state & T_RECALLED)
			debug("RECALLED ");
		if(tp->t_state & T_KSLEEPING)
			debug("KSLEEP(%x,%d) ", tp->t_wait.t_sleep.t_saddr,
					    tp->t_wait.t_sleep.t_syncn);
		if(tp->t_state & T_KREQUEST)
			debug("KREQUEST ");
		debug("\n   seg=%x  dom=%x  dw<=%x  dw>=%x  dwseg=%x\n",
			tp->t_smap.m_seg, tp->t_subdom,
			tp->t_dwbase, tp->t_dwbound, tp->t_dwmap.m_seg);
	}
}


/*
 *  spawn a new thread
 *  initially simulate a call on a port to the current domain whose
 *    handler is (*fun)() so that when the thread starts running it
 *    starts in the desired place
 *  when the call returns the thread will be garbage collected
 */

thread_t *
t_spawn(parent, dom, fun, pass, pri, op)
thread_t  *parent;
subdom_t  *dom;
int	(*fun)();
arg_t	op;
{
	int	i;
	register thread_t  *tp;

	for(tp = &threads[0]; tp < &threads[THREADS]; tp++) {
		if(tp->t_state == T_FREE) {
			tp->t_state = T_INUSE|T_RUNABLE;

			/* thread initially has no children */
			l_list(&tp->t_children);
			/* link it into activity group */
			if((tp->t_parent = parent) != NULLTHREAD) {
				/* link this thread into sibling list */
				l_inlink(&tp->t_parent->t_children,
					 &tp->t_sibling);
			}

			/* put it in the domains scheduling list */
			l_init(&tp->t_schdlnk);
			tp->t_subdom = dom;
			t_sched(tp, &(dom->d_domain->d_schdlst), pri);

			/* set stack up */
			if((tp->t_smap.m_seg = s_alloc(stacksize, S_STACK)) ==
			   NULLSEGMENT)
				panic("can't allocate stack segment");
			/* start with the entire stack mapped in */
			tp->t_smap.m_addr = STACKBASE;
			tp->t_stacktop = STACKTOP-4;
			tp->t_usp = STACKTOP-4;

			/* set up NULL data window */
			tp->t_dwmap.m_seg = NULLSEGMENT;
			tp->t_dwmap.m_addr = DWBASE;
			tp->t_dwbase = DWBASE;
			tp->t_dwbound = DWBASE;

			/* set up args of spawned thread */
			tp->t_opcode = 0;
			tp->t_msg.m_opcode = op;
			tp->t_args[JUMP_ENTRY] = (arg_t)fun;
			tp->t_args[JUMP_PASSPORT] = (arg_t)pass;

			if(SYSSUBDOM(dom))
				tp->t_upc = (funp_t)KERNBASE;
			else
				tp->t_upc = (funp_t)USERBASE;
			tp->t_ups = 0;	/* user mode */
			tp->t_except.ex_type = 0;	/* simple rte */

			/*
			 *  on a close op we must save the closed domain
			 *    otherwise a jump might loose the information
			 *  this could be obviated (ill get to it later)
			 */
			if(op == CLOSE)
				tp->t_close = dom->d_domain;
			return(tp);
		}
	}
	panic("out of threads");
	return(NULLTHREAD);
}


/*
 *  recall all the children of the specified thread
 *  there is an assumption that T_CURRENT is not signaled by this
 *    (it should be the thread being passed)/
 *
 */

t_recall(tp)
register thread_t  *tp;
{
	register struct link  *l;
	register thread_t  *t;

	/* recall the activity group of the thread */
	FOREACHLINK(l, tp->t_children) {
		t = LINKREF(l, t_sibling, thread_t *);
		/* recurse down thread to find "active" point */
		while(t->t_rstack != NULLTHREAD) {
			/* recall of intermediate levels is delayed */
			t->t_state |= T_RECALLED;
			t = t->t_rstack;
		}
		/*
		 *  this is a leaf of the top level activity group
		 *  if this thread has children here (either local real ones
		 *    or phantom ones across a network) the handling of the
		 *    signal will either:
		 *     explicitly forward the recall to the children
		 *      (by doing an actual recall??)
			[hmm not so good since its then in the handler
			 who cant recall the up-one-level children]
		 *     implicitly do the recall by detaching the tree as a
		 *      side effect of a reply
		 *  in the network case the local leaf has to do the explicit
		 *    forward to the rest of the activity on the remote
		 *    machine
		 */
		t_signal(t, EXCEPT_RECALL);
	}
}


/* force exception request in thread */

thread_t *
t_signal(tp, ex)
register thread_t  *tp;
{
	extern	thread_t  *sysreply();

	debug("sending exception %d to thread %x\n", ex, tp);
	if(tp->t_state & (T_SLEEPING | T_KSLEEPING)) {
		/* its asleep, do a wakeup */
		tp->t_state |= T_RECALLED;
		t_wakeup(tp, RECALL);
	}
	else if(tp->t_state & T_KREQUEST) {
		/* threads in the midst of a KREQUEST ignore recalls */
	}
	else if(SYSSUBDOM(tp->t_subdom)) {
		/* threads in the system domain ignore recalls (for now...) */
		tp->t_state &= ~T_RECALLED;
	}
	else {
		/* a thread running in a normal user domain */
		tp->t_state &= ~T_RECALLED;

		/* force a reply if there is no recall handler */
		/* make sure nothing is funny in the replycode */
		tp->t_msg.m_ercode = E_OK;
		tp = sysreply(tp);
		if(tp == NULLTHREAD)
			debug("THREAD UNWOUND ALL THE WAY\n");
		return(tp);
	}

	return(tp);
}


/* wakeup the thread setting its sleep status */

t_wakeup(tp, status)
register thread_t  *tp;
{
	tp->t_wait.t_wakeup = status;

	tp->t_state &= ~(T_SLEEPING|T_KSLEEPING);
	tp->t_state |= T_RUNABLE;

	/* take domain out of sleep list */
	l_unlink(&tp->t_wait.t_sleep.t_slnk);

	/* reschedule the thread */
	if(tp->t_state & T_KREQUEST)
		/* delay putting the thread back in the domains list */
		t_sched(tp, &(kcalllist), tp->t_priority);
	else
		t_sched(tp, &(tp->t_subdom->d_domain->d_schdlst), tp->t_priority);
}


/* take a thread out of its current scheduling list */

t_unsched(tp)
register struct thread  *tp;
{
	ASSERT(tp->t_subdom != NULLSUBDOM);

	/* unlink the thread from the scheduling list */
	l_unlink(&tp->t_schdlnk);
}


/* link a thread into a new domain scheduling list */

t_sched(tp, tl, pri)
register struct thread  *tp;
register struct list    *tl;
{
	register struct link  *l;

	/* set domain and priority of domain */
	tp->t_priority = pri;

	/* link thread into the priority ordered scheduling list */
	FOREACHLINK(l, (*tl)) {
		if(LINKREF(l, t_schdlnk, thread_t *)->t_priority <
		   tp->t_priority)
			break;
		tl = (struct list *)l;
	}
	l_link(tl, &tp->t_schdlnk);
}


/*
 *  remove the thread from its current scheduling list and enter it in
 *    the new one
 */

t_resched(tp, tl, pri)
register struct thread  *tp;
register struct list  *tl;
{
	register struct link  *l;

	/* unlink the thread from the scheduling list */
	l_unlink(&tp->t_schdlnk);

	/* set priority of domain */
	tp->t_priority = pri;

	/* link thread into the priority ordered scheduling list */
	FOREACHLINK(l, (*tl)) {
		if(LINKREF(l, t_schdlnk, thread_t *)->t_priority <
		   tp->t_priority)
			break;
		tl = (struct list *)l;
	}
	l_link(tl, &tp->t_schdlnk);
}


/* free the specified thread */

t_free(tp)
register thread_t  *tp;
{
	register thread_t  *ntp;
	register struct link  *l;

	/* free stack segment */
	s_free(tp->t_smap.m_seg);

	/* make all children threads parentless (detatched) */
	FOREACHLINK(l, tp->t_children) {
		/* should probably send detatch recall */
		LINKREF(l, t_sibling, thread_t *)->t_parent = NULLTHREAD;
		/* CANNOT l_init out the sibling link but its unesessary */
	}

	/* remove from sibling list of its parent*/
	if(tp->t_parent != NULLTHREAD) {
		l_unlink(&tp->t_sibling);
	}

	/* free the thread */
	*tp = threads[THREADS];
}


ERROR(e)
{
	printf("ERROR(%d)\n", e);
	T_CURRENT->t_msg.m_ercode = e & OP_OPCODE;
}


/* do a push on the thread stack (for a request) */

thread_t *
t_push(tp)
register thread_t  *tp;
{
	register thread_t  *cp;
	static	 thread_t  *lp = &threads[0];

	/* allocate new stack element to store what is now the top element */
led_on();
	cp = lp;
	do {
		if(++cp >= &threads[THREADS])
			cp = &threads[0];
		if(cp->t_state == T_FREE) {
			/* start searching here next time */
			lp = cp;

			/* setup the state of the new element */
			cp->t_state = T_INUSE|T_RUNABLE;
			cp->t_smap = tp->t_smap;
			cp->t_subdom = tp->t_subdom;

			/* copy old data window for use during jump */
			cp->t_dwmap = tp->t_dwmap;
			cp->t_dwbase = tp->t_dwbase;
			cp->t_dwbound = tp->t_dwbound;

			/* copy the args into the new element */
			cp->t_data = tp->t_data;
/*			mcopy(tp->t_args, cp->t_args, sizeof(tp->t_args));*/
 
			/* link rest of stack onto head */
			cp->t_stack = tp;
			/* this link is used to search from the stack base */
			tp->t_rstack = cp;
			tp->t_state |= T_REQUEST;

			/* the thread hasn't used any time */
			tp->t_time = 0;

			/* initialize the scheduling list link */
			l_init(&cp->t_schdlnk);
led_off();
			return(cp);
		}
	}  while(cp != lp);
	panic("can't allocate request stack");
}


/*  pop the thread request stack (for a reply) */

thread_t *
t_pop(tp)
register thread_t  *tp;
{
	register thread_t  *cp;
	register struct link  *l;

	cp = tp->t_stack;
	ASSERT(cp->t_state & T_REQUEST);
	cp->t_state &= ~T_REQUEST;
	cp->t_rstack = NULLTHREAD;

	/* reflect some state back up to the parent */
	cp->t_time += tp->t_time;

	/* copy the args back up */
	mcopy(tp->t_args, cp->t_args, sizeof(tp->t_args));

	/* make all children threads parentless (detatched) */
	FOREACHLINK(l, tp->t_children) {
		/* should probably send detatch recall */
		LINKREF(l, t_sibling, thread_t *)->t_parent = NULLTHREAD;
		/* could l_init out the sibling link but its unesessary */
	}

	/* free the old element */
	*tp = threads[THREADS];
	return(cp);
}
