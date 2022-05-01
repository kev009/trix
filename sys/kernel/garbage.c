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
#include	"../h/com.h"
#include	"../h/global.h"
#include	"../h/domain.h"
#include	"../h/memory.h"
#include	"../h/thread.h"
#include	"../h/assert.h"
#include	"../h/args.h"


/*
 *  this routine will find all the active domains and do a transitive
 *    closure over the handles they own
 *  any domain that is not part of this closure is then freed
 */

gc_domain()
{
	register handle_t  *hp;
	register domain_t  *dp;
	register thread_t  *tp;

printf("starting gc\n");
	/* build handle list for each domain */

	for(hp = &handles[0] ; hp < &handles[HANDLES] ; hp++)
		if(hp->h_port != NULLPORT) {
			/* its allocated - put it in list */
			hp->h_handle = hp->h_domain->d_hlist;
			hp->h_domain->d_hlist = hp;
		}
printf("made hlist\n");

	/* mark and propagate marks */
	for(tp = &threads[0] ; tp < &threads[THREADS] ; tp++)
		if(tp->t_state != T_FREE) {
			ASSERT(tp->t_domain != NULLDOMAIN);
			dp = tp->t_domain;
			if(!(dp->d_state & D_MARK)) {
				printf("domain %x is active\n", dp);
				dp->d_state |= D_MARK;
				gc_hprop(dp);
			}
		}

printf("all marks propagated\n");
	/* clear all marks and free any unmarked domains */
	for(dp = &domains[0] ; dp < &domains[DOMAINS] ; dp++) {
		/* clear handle list */
		dp->d_hlist = NULLHANDLE;
		if(dp->d_state) {
			if(!(dp->d_state & D_MARK)) {
				/* free the sucker */
				printf("i would like to gc domain %x\n", dp);
			}
			else {
				printf("no gc of domain %x\n", dp);
				dp->d_state &= ~D_MARK;
			}
		}
	}
printf("gc done\n");
}


gc_hprop(fdp)
register domain_t  *fdp;
{
	register struct domain  *dp;
	register struct handle  *hp;

printf("gc_hprop(%x)\n", fdp);
	for(hp = fdp->d_hlist ; hp != NULLHANDLE ; hp = hp->h_handle) {
		dp = hp->h_port->p_domain;
		if(!(dp->d_state & D_MARK)) {
printf(" following h=%x  from %x to %x\n", hp, fdp, dp);
			/* mark it and continue recurse */
			dp->d_state |= D_MARK;
			gc_hprop(dp);
		}
		else
printf(" following h=%x  from %x to *%x\n", hp, fdp, dp);
	}
}
