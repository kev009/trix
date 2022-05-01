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


/* validate the opcode and the target and passed handles */

rqst_valid(tp)
register thread_t  *tp;
{
	if(tp->t_msg.m_opcode & OP_SYSTEM) {	/* block reserved opcode */
		printf("RESERVED OPCODE %x\n", tp->t_msg.m_opcode);
		tp->t_msg.m_ercode = E_OPCODE;
		return(0);
	}

	if(!h_legal((hndl_t)tp->t_target)) {	/* validate the target */
		printf("  bad target\n");
		tp->t_msg.m_ercode = E_TARGET;
		return(0);
	}

	debug("  dom=%x  entry=%x  pass=%x\n", tp->t_subdom->d_domain,
		tp->t_entry, tp->t_passport);

	if(tp->t_msg.m_opcode & OP_HANDLE) {	/* validate the passed handle */
		if(!h_legal((hndl_t)tp->t_msg.m_handle)) {
			printf("  bad passed handle\n");
			tp->t_msg.m_ercode = E_HANDLE;
			return(0);
		}
	}

	return(1);
}


/* do the actual domain jump after setting up the data window */

rqst_jump(tp, jmp)
register thread_t  *tp;
{
	register domain_t  *dcp = tp->t_subdom->d_domain;
	port_t   *pp;

	/*
	 * the current thread wishes to run in the specified domain:
	 *  save the current registers on the request stack of the thread
	 *  set up the saved registers so a return from kernel will start
	 *   running at the entry point of the specified domain
	 * map in the domain (mapping out the current one)
	 */

	/* the data window is always mapped in at DWBASE */
	tp->t_dwmap.m_addr = DWBASE;

	/* remember the request being done (for OP_DWTYPE in fetch/store) */
	tp->t_opcode = tp->t_msg.m_opcode;

	/* if no data-window in request do jump */
	if((tp->t_msg.m_opcode & OP_DWTYPE) == 0 || (off_t)tp->t_msg.m_dwsize == 0) {
		tp->t_dwmap.m_seg = NULLSEGMENT;
		tp->t_dwbase = DWBASE;
		tp->t_dwbound = DWBASE;
		goto jump;
	}

	/* there is a readable data window, force a cache flush */
	if(tp->t_opcode & DW_READ)
		flushcache = 1;

	/* check accessability of data-window in old data window */
	if(tp->t_msg.m_dwaddr >= tp->t_dwbase &&
	   (tp->t_msg.m_dwaddr + tp->t_msg.m_dwsize) <= tp->t_dwbound) {
		/* forwarding a pointer to the old data window */
		/* the segment stays the same */
		/* the addr stays the same (DWBASE) */
		/* the base is the new address */
		tp->t_dwbase = tp->t_msg.m_dwaddr;
		/* the bound is the base plus the size */
		tp->t_dwbound = tp->t_dwbase + tp->t_msg.m_dwsize;
		goto jump;
	}

	/* check accessability of data-window in thread stack segment */
	if(map_within(&tp->t_smap, tp->t_msg.m_dwaddr, tp->t_msg.m_dwsize)) {
		tp->t_dwmap.m_seg = tp->t_smap.m_seg;
		tp->t_dwbase = DWBASE +	(tp->t_msg.m_dwaddr - tp->t_smap.m_addr);
		tp->t_dwbound = tp->t_dwbase + (off_t)tp->t_msg.m_dwsize;
		goto jump;
	}

	/* check accessability of data-window in current domain segment */
	if(map_within(&dcp->d_tdmap, tp->t_msg.m_dwaddr, tp->t_msg.m_dwsize)) {
		tp->t_dwmap.m_seg = dcp->d_tdmap.m_seg;
		tp->t_dwbase = DWBASE +	(tp->t_msg.m_dwaddr - dcp->d_tdmap.m_addr);
		tp->t_dwbound = tp->t_dwbase + (off_t)tp->t_msg.m_dwsize;
		goto jump;
	}

	/* if data-window is not accessable set error code and return */
	printf("bad data window: %x %x  op = %x\n",
		tp->t_msg.m_dwaddr, tp->t_msg.m_dwsize, tp->t_msg.m_opcode);
	tp->t_msg.m_ercode = E_WINDOW;
	return(0);

jump:
	/*
	 * pass handler the window base to use as base in fetch/store calls
	 * or when forwarding a window in a request
	 */
	tp->t_msg.m_dwsize = tp->t_dwbound - tp->t_dwbase;
	tp->t_msg.m_dwbase = tp->t_dwbase;

	pp = h_port((hndl_t)tp->t_target);

	/* setup entry and passport (passport replaces TARGET) */
	tp->t_entry = pp->p_entry;
	tp->t_passport = (arg_t)pp->p_passport;

	debug("  dom=%x  entry=%x  pass=%x\n", tp->t_subdom->d_domain,
		tp->t_entry, tp->t_passport);

	if(SYSSUBDOM(pp->p_subdom))
		tp->t_upc = (funp_t)KERNBASE;
	else
		tp->t_upc = (funp_t)USERBASE;
	/* set up psw */
	tp->t_ups = 0;
	/* set up rstack to resume in new domain */
	tp->t_subdom = pp->p_subdom;

	if(!jmp) {
		/* keep it in schdlst without RUNNABLE if priority sticks */
		t_resched(tp->t_stack,
			  &(tp->t_stack->t_subdom->d_domain->d_access),
			  tp->t_stack->t_priority);
		t_sched(tp, &(pp->p_subdom->d_domain->d_schdlst), pp->p_priority);
	}
	else
		t_resched(tp, &(pp->p_subdom->d_domain->d_schdlst), pp->p_priority);

	if(tp->t_msg.m_opcode & OP_HANDLE)  /* attach passed handle to new domain */
		h_attach((hndl_t)tp->t_msg.m_handle, tp->t_subdom);

	return(1);
}
