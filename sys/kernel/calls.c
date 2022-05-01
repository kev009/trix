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
#include 	"../h/memory.h"
#include	"../h/domain.h"
#include	"../h/thread.h"
#include	"../h/args.h"
#include	"../h/assert.h"


/* this is a place holder in the calls table */

thread_t *
syserror(tp)
thread_t  *tp;
{
	panic("syserror called");

	/* continue running the same thread */
	return(tp);
}


/*
 * creat a new subdomain and switch the current thread into it
 * a subdomain is used to decrease the granularity of GC of resources
 * a single handle can be passed across the interface
 */

thread_t *
syssubdom(tp)
register thread_t  *tp;
{
	printf("** SYSSUBDOM\n");
	/* creat the new subdomain as part of the calling domain */
	/* tp->t_subdom = newsubdom; */
	/* attach the passed handle to the new subdomain */

	/* continue running the same thread */
	return(tp);
}


/*
 * spawn a new thread which will run in the current domain
 *   starting at its entry location.
 * it is differentiated from threads doing calls into the domain
 *   by its dp being NULL.
 */

thread_t *
sysspawn(tp)
register thread_t  *tp;
{
	debug("** SYSSPAWN\n");
	if(t_spawn(tp, tp->t_subdom,
		tp->t_args[PORT_ENTRY],
		tp->t_args[PORT_PASSPORT],
		tp->t_args[PORT_PRIORITY], 0) == NULLTHREAD)
			ERROR(E_SPAWN);

	/* continue running the same thread */
	return(tp);
}


/* reply to a request */

thread_t *
sysreply(tp)
register thread_t  *tp;
{
	register caddr_t  smap;

	debug("** SYSREPLY(%x): ", tp->t_msg.m_ercode);
	if(tp->t_msg.m_ercode & OP_HANDLE) {
		if(!h_legal((hndl_t)tp->t_msg.m_handle))
			/* ok so whatcha gonna do about it */
			tp->t_msg.m_ercode &= ~OP_HANDLE;
	}

	t_unsched(tp);

	/* if the thread has children do a recall first */
	if(!EMPTYLIST(tp->t_children))
		t_recall(tp); 

	if(tp->t_stack == NULLTHREAD) {
		thread_t  *ntp;

		/* no return stack -- thread is freed */
		/* t_free MUST free the threads resources */
		debug("final reply from thread: %x\n", tp);
		if(tp->t_msg.m_ercode & OP_HANDLE) {
			h_close((hndl_t)tp->t_msg.m_handle);
			debug("  RF_HANDLE=%X\n", tp->t_msg.m_handle);
		}

		if(tp->t_close != NULLDOMAIN) {
			register struct link  *l;
			/*
			 *  there is no longer a close thread
			 *  if there are handles waiting to be closed start a
			 *    new thread on the first one
			 */
			tp->t_close->d_state &= ~D_CLOSE;
			l = l_first(&tp->t_close->d_pdead);
			debug("close thread returning, now try %x\n", l);
			if(l != NULLLINK)
				p_close(LINKREF(l, p_link, port_t *));
		}
 
		/* free the old thread */
		t_free(tp);
		/* there is no thread left (trap will call schedule()) */
		return(NULLTHREAD);
	}

	/* pop the request stack */
	smap = tp->t_smap.m_addr;

	/* there was a writeable data window, force a cache flush */
	if(tp->t_opcode & DW_WRITE)
		flushcache = 1;

	tp = t_pop(tp);
	t_resched(tp, &(tp->t_subdom->d_domain->d_schdlst), tp->t_priority);
	if(smap != tp->t_smap.m_addr)	/* the stack has to be remapped */
		T_MAPPED = NULLSEGMENT;

	debug("dom=%x\n", tp->t_subdom);
	if(tp->t_msg.m_ercode & OP_HANDLE)
		h_attach((hndl_t)tp->t_msg.m_handle, tp->t_subdom);

	if(tp->t_msg.m_ercode & OP_FERROR) {
		/* if OP_FERROR is set we should force a signal here */
		printf("OP_ERROR in reply ignored\n");
	}

	/* continue running the thread */
	return(tp);
}


/*
 * whenever a thread does a jump into a new domain its stack is
 *   initialized to STACKTOP-4 (well not anymore...)
 * in general there will normally be some data structure allocated
 *   here (top of stack) by the user level entrance to the domain
 * it will contain the equivalent of a unix U area so you can tell
 *   what thread you are (initialized with (fun,data) from the call)
 * this allows it to be mapped automatically when the thread is scheduled
 *   (the only other place it could be would be a register and thats
 *    too constraining a convention)
 */

thread_t *
sysrequest(tp)
register thread_t  *tp;
{
	port_t   *pp;

	debug("** SYSREQUEST(%d,%x): ",	tp->t_target, tp->t_msg.m_opcode);

	/* make sure the request is a legal user request */
	if(!rqst_valid(tp))
		return(tp);

	/* see if the request is on a kernel port */
	pp = h_port((hndl_t)tp->t_target);
	if(pp->p_state & P_KERNEL) {
		/* short circuit request on KERNEL port */
		debug("jump onto a kernel port %x  e=%x p=%x\n",
			pp, pp->p_entry, pp->p_passport);
	
		tp->t_passport = pp->p_passport;
		tp->t_entry = pp->p_entry;
		if((*(funp_t)tp->t_entry)(tp->t_passport, &tp->t_msg) != 0) {
			/* 
			 *  a return of 1 means the request is not completed
			 *    trap() will rerun it later (with the KREQUEST flag set)
			 *  a return of 0 means the request has completed execution
			 *    it may not be RUNNABLE (if for example it went to sleep)
			 */
			tp->t_state |= T_KREQUEST;
			debug("KREQUEST doesn't return 0!!\n");
		}

		return(tp);
	}


	/* calculate the new smap for ustack */
	/* push the request stack to allow return */
	tp = t_push(tp);

	if(!rqst_jump(tp, 0)) {
		tp = t_pop(tp);
		return(tp);
	}

	/*
	 * now do the actual remapping of the stack
	 * we have to do the jump first because of problems calculating
	 *   the window information (it refers to the old mapping)
	 * for now the stack is shifted but not protected (at the high end)
	 */
	tp->t_stacktop = tp->t_usp = tp->t_stack->t_usp;
	/* set up the new smap */
	/* force the stack to be remapped (if it was moved) */

	/* continue running the same thread */
	return(tp);
}


/* jump to a new domain instead of doing a call */

thread_t *
sysrelay(tp)
register thread_t  *tp;
{
	register subdom_t *dcp = tp->t_subdom;
	port_t   *pp;

	/*
	 * a relay can either have a forwarded data window or
	 *  a window into the thread stack segment
	 *   the latter is crutial to allow exec to pass args without keeping
	 *    the domain alive, otherwise you need a handshake.
	 *   it is also crutial that a series of execs dont keep making the
	 *    thread stack longer by piling up copies of the arguments
	 */
	
	debug("** SYSRELAY(%d,%x): ",
		tp->t_target, tp->t_msg.m_opcode);

	/* make sure the request is a legal user request */
	if(!rqst_valid(tp))
		return(tp);

	/* check accessability of data-window in current domain segment */
	if(map_within(&dcp->d_domain->d_tdmap,
		      tp->t_msg.m_dwaddr,
		      tp->t_msg.m_dwsize)) {
		/* this is illegal in a relay */
		printf("relay with domain window\n");
		tp->t_msg.m_ercode = E_WINDOW;
		return(tp);
	}

	/* see if the relay is on a kernel port */
	pp = h_port((hndl_t)tp->t_target);
	if(pp->p_state & P_KERNEL) {
		/* short circuit request on KERNEL port */
		debug("relay onto a kernel port %x  e=%x p=%x\n",
			pp, pp->p_entry, pp->p_passport);
	
		if((*(funp_t)pp->p_entry)(pp->p_passport, &tp->t_msg) != 0) {
			/* 
			 *  a return of 1 means the request is not completed
			 *    trap() will rerun it later (with the KREQUEST flag set)
			 *  a return of 0 means the request has completed execution
			 *    it may not be RUNNABLE (if for example it went to sleep)
			 */
			tp->t_state |= T_KREQUEST;
			panic("KRELAY doesn't return 0!!\n");
		}

		return(sysreply(tp));
	}

	rqst_jump(tp, 1);

	/* reset stack pointer */
/*	tp->t_usp = tp->t_stacktop;*/

	/* continue running the same thread */
	return(tp);
}


/* send a recall exception to the activity group of the caller */

thread_t *
sysrecall(tp)
thread_t  *tp;
{
	debug("** SYSRECALL %x\n", tp);
	t_recall(tp);
	tp->t_args[R_OPCODE] = E_OK;

	/* continue running the same thread */
	return(tp);
}


/* read from the data window of the current request */

thread_t *
sysfetch(tx)
register thread_t  *tp;
{
	register arg_t	 c = tp->t_args[M_DWSIZE];
	register caddr_t o = (caddr_t)tp->t_args[M_PARAM0];
	register caddr_t s = (caddr_t)tp->t_args[M_DWADDR];

	debug("** SYSFETCH(%x,%d,%x)\n", o, c, tp->t_dwbase);
	if(o < tp->t_dwbase || o >= tp->t_dwbound) {
		if(o != tp->t_dwbase || c != 0) {
			/* access of users own address space */
			panic("direct fetch not supported yet");
		}
	}

	/* normal access through data window pointer */
	if(c > 0) {
		if(o + c > tp->t_dwbound)
			c = tp->t_dwbound - o;
/*		s_read(c, s, tp->t_dwmap.m_seg, o - tp->t_dwmap.m_addr);*/
		mcopy(o, s, c);
	}

	tp->t_args[R_OPCODE] = E_OK;
	tp->t_args[R_PARAM0] = c;

	/* continue running the same thread */
	return(tp);
}


/* write to the data window of the current request */

thread_t *
sysstore(tp)
register thread_t  *tp;
{
	register arg_t	 c = tp->t_args[M_DWSIZE];
	register caddr_t o = (caddr_t)tp->t_args[M_PARAM0];
	register caddr_t s = (caddr_t)tp->t_args[M_DWADDR];

	debug("** SYSSTORE(%x,%d,%x)\n", o, c, tp->t_dwbase);
	if(o < tp->t_dwbase || o >= tp->t_dwbound) {
		if(o != tp->t_dwbase || c != 0) {
			/* access of users own address space */
			panic("direct store not supported yet");
		}
	}

	/* normal access through data window pointer */
	if(c > 0) {
		if(o + c > tp->t_dwbound)
			c = tp->t_dwbound - o;
/*		s_write(c, s, tp->t_dwmap.m_seg, o - tp->t_dwmap.m_addr);*/
		mcopy(s, o, c);
	}

	tp->t_args[R_OPCODE] = E_OK;
	tp->t_args[R_PARAM0] = c;

	/* continue running the same thread */
	return(tp);
}


/*
 *  return the index for a newly created port on the calling domain
 *  this is a call (rather than a domain_handler request) to support
 *    subdomains later
 */

thread_t *
sysmakeport(tp)
register thread_t  *tp;
{
	hndl_t  h;
	funp_t	entry = (funp_t)tp->t_args[PORT_ENTRY];
	arg_t	passport = tp->t_args[PORT_PASSPORT];
	arg_t	priority = tp->t_args[PORT_PRIORITY];

	debug("** SYSMAKEPORT(%x,%x) --> ", entry, passport);
	h = p_alloc(tp->t_subdom, entry, passport, priority);
	if(h != NULLHNDL) {
		tp->t_args[R_OPCODE] = (OP_HANDLE|E_OK);
		tp->t_args[R_HANDLE] = (arg_t)h;
		debug("%d\n", h);
	}

	/* continue running the same thread */
	return(tp);
}


/* duplicate a handle and return index to the new handle */

thread_t *
sysdup(tp)
register thread_t  *tp;
{
	hndl_t  h = (hndl_t)tp->t_args[M_HANDLE];

	debug("** SYSDUP(%d) -->", h);
	if(!h_legal(h)) {
		ERROR(E_HANDLE);
		return(tp);
	}

	/* allocate new handle on same port	*/
	h = h_alloc(h_port(h), tp->t_subdom);
	if(h != NULLHNDL) {
		tp->t_args[R_OPCODE] = (OP_HANDLE|E_OK);
		tp->t_args[R_HANDLE] = (arg_t)h;
		debug("%d\n", h);
	}

	/* continue running the same thread */
	return(tp);
}


/*
 *  set the priority (preemptability) of the thread in the current
 *    request frame, return the old priority.
 */

thread_t *
sysspl(tp)
register thread_t  *tp;
{
	int	oldpriority;

	debug("** SYSSPL()\n");
	oldpriority = tp->t_priority;

	/* reschedule the thread */
	t_resched(tp,
		  &(tp->t_subdom->d_domain->d_schdlst),
		  tp->t_args[THREAD_PRIORITY]);

	tp->t_args[R_OPCODE] = E_OK;
	tp->t_args[R_PARAM0] = oldpriority;

	/* continue running the same thread */
	return(tp);
}


/*
 *  close the handle
 *  if its the last handle to the referenced port spawn a detached thread
 *    to do a CLOSE request and delete the port.
 */

thread_t *
sysclose(tp)
register thread_t  *tp;
{
	hndl_t  h = (hndl_t)tp->t_args[M_HANDLE];

	debug("** SYSCLOSE()\n");
	if(!h_legal(h)) {
		printf("close of bad handle\n");
		tp->t_args[R_OPCODE] = E_HANDLE;
		return(tp);
	}

	h_close(h);
	tp->t_args[R_OPCODE] = E_OK;

	/* continue running the same thread */
	return(tp);
}


/*
 *  fold a handle into an internal form for use by the domain that created
 *    the port
 *  if the handle is folded it is also closed
 */

thread_t *
sysfold(tp)
register thread_t  *tp;
{
	hndl_t  h = (hndl_t)tp->t_args[M_HANDLE];
	port_t	*pp;

	debug("** SYSFOLD(%d) --> ", h);
	if(!h_legal(h)) {
		printf("fold of bad handle\n");
		tp->t_args[R_OPCODE] = E_HANDLE;
		return(tp);
	}

	pp = h_port(h);
	if(pp->p_subdom == tp->t_subdom) {
		tp->t_args[PORT_ENTRY] = (arg_t)pp->p_entry;
		tp->t_args[PORT_PASSPORT] = (arg_t)pp->p_passport;
		tp->t_args[R_OPCODE] = E_OK;
		debug("%x %x\n", tp->t_args[PORT_ENTRY],
			tp->t_args[PORT_PASSPORT]);
	}
	else {
		tp->t_args[R_OPCODE] = E_FOLD;
		debug("E_FOLD\n");
	}

	/* continue running the same thread */
	return(tp);
}
