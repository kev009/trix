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

#include	"traptab.d"


#define	PS_TRACE	0100000	/* trace bit */


/*
 * trap() and ktrap() are called from low.s when a processor trap occurs.
 * The arguments are the words saved on the system stack by the hardware
 *   and software during the trap processing.
 * The order is dictated by the hardware and details of C's calling sequence.
 * They are peculiar in that this call is not 'by value' and changed user
 *   registers get copied back on return.
 * Number is the kind of trap that occurred.
 */


trap(number)
short	number;
{
	register thread_t *tc = T_CURRENT;

	ASSERT(tc != NULLTHREAD);

	/* mark the thread as running in the kernel */
	tc->t_state |= T_KERNEL;

	/*
	 *  the kernel call runs at priority >= SPL_WAKEUP
	 *  in this way the call is viewed as atomic (at least w.r.t. wakeups)
	 */
	if((tc->t_ups & 0x700) < SPL_WAKEUP)
		hplx(SPL_WAKEUP | SMODE);

	switch(number) {
	    case 2:
		/* bus error */
		if(page_fault(tc))
			/* we were able to deal with the page fault */
			break;
		/* unrecoverable fault (drops through) */
	    case 3:
		/* address error */
		printf("address error:  (pc=%x,sp=%x,a=%x,i=%x,f=%x)\n",
		       tc->t_upc, tc->t_usp,
		       tc->t_except.ex_faddr,
		       tc->t_except.ex_inst,
		       tc->t_except.ex_ssw);
		prtargs();
		if(!SYSSUBDOM(tc->t_subdom)) {
			printf("sending recall signal to thread\n");
			tc = t_signal(tc, EXCEPT_BUSERR);
		}
		else
			panic("I quit\n");
		break;

	    case 4:
		/* illegal instruction */
		printf("illegal instruction:  (pc=%x,*pc=%x,sp=%x)\n",
			tc->t_upc, *(short *)(tc->t_upc), tc->t_usp);
		prtargs();
		if(!SYSSUBDOM(tc->t_subdom)) {
			printf("sending recall signal to thread\n");
			tc = t_signal(tc, EXCEPT_MATH);
		}
		else
			panic("I quit\n");
		break;

	    case 9:
		/* trace trap */
		printf("trace:  (pc=%x,*pc=%x,sp=%x)",
			tc->t_upc, *(short *)(tc->t_upc), tc->t_usp);
		if (tc->t_watch != NULL)
			printf(" *w=%x\n", *(int *)(tc->t_watch));
		else
			printf("\n");
		break;

	    case 32:
	    {	arg_t	kc = T_args[KERCALL];

		if(kc < 0 || kc >= sizeof(traptab)/sizeof(traptab[0]))
			panic("unknown kernel call %d\n", kc);

		/* dispatch on trap type */
		tc = (*traptab[kc])(tc);
		break;
	    }

	    case 33:
		debug("tracing %x on\n", tc);
		tc->t_ups |= PS_TRACE;
		break;

	    case 34:
		debug("tracing %x off\n", tc);
		tc->t_ups &= ~PS_TRACE;
		break;

	    case 35:
	    case 256:
		/* reschedule the processor */
		dosched++;
		break;

	    case 36:
	    {	extern	int	Debug;

		Debug = 1;
		debug("debug printout on\n");
		break;
	    }

	    case 37:
	    {	extern	int	Debug;

		debug("debug printout off\n");
		Debug = 0;
		break;
	    }

	    case 38:
		debug("profon\n");
		break;

	    case 39:
		debug("profoff\n");
		s_free(s_alloc(stacksize, S_STACK));
		s_free(s_alloc(stacksize, S_STACK));
/*		t_pop(t_push(tc));t_pop(t_push(tc));
		t_pop(t_push(tc));t_pop(t_push(tc));
		t_pop(t_push(tc));t_pop(t_push(tc));
		t_pop(t_push(tc));t_pop(t_push(tc));*/
/*		resume();resume();resume();resume();
		resume();resume();resume();resume();*/
		break;

	    case 40: case 41: case 42: case 43:
	    case 44: case 45: case 46: case 47:
		panic("unknown user trap type %d", number);
		break;

	    default:
		panic("unknown user trap type %d  (pc=%x,sp=%x)",
		      number, tc->t_upc, tc->t_usp);
		break;
	}

	/*
	 *  find the next runnable thread and map it (and its domain) in
	 *  if it is in the middle of a KREQUEST rerun it and (if its not done)
	 *    loop through
	 *  if it has a signal pending, send the signal
	 */
	for( ; ; ) {
	    /* loop looking for a thread that isn't running in the KERNEL */
	    do {
		/* the current thread needs to be rescheduled */
		if(dosched || tc == NULLTHREAD ||
		   l_first(&tc->t_subdom->d_domain->d_schdlst) != &tc->t_schdlnk) {
			/* were not first in our domain */
			tc->t_state &= ~T_RUNNING;
			tc = schedule();
			dosched = 0;
		}

		/* restore state of current thread */
		T_CURRENT = tc;
		memmap(T_CURRENT);
		tc->t_state |= T_RUNNING;

		/* if the thread is doing a KREQUEST run it (a second time) */
	    }  while((tc->t_state & T_KREQUEST) &&
		     (*(funp_t)tc->t_entry)(tc->t_passport, &tc->t_msg) != 0);

	    /* clear KREQUEST (if it was set) */
	    tc->t_state &= ~T_KREQUEST;

	    /* deal with recalled thread returning */
	    if(tc->t_state & T_RECALLED) {
		/* send the signal and loop again */
		tc = t_signal(tc, EXCEPT_RECALL);
		continue;
	    }
	    if(l_first(&tc->t_subdom->d_domain->d_schdlst) == &tc->t_schdlnk)
		break;
	}

	/* at this point the thread is mapped into the domain and ready to run */

	/* the thread is done running in the kernel */
	tc->t_state &= ~T_KERNEL;

	ASSERT(tc->t_state & T_RUNABLE);
	/* this is here for assembler use in spl emulation */
	{	extern domain_t	*D_CURRENT;
		D_CURRENT = tc->t_subdom->d_domain;
	}

	debug("trap returning to %x:%x\n", tc->t_upc, tc->t_usp);
}


/* handle traps from kernel mode -- most are fatal errors */

ktrap(number, d0, d1, a0, a1, frame)
short	number;
struct {
	long	psw;		/* aligned by trap number for now */
	long	pc;
	short	type;
	short	fcode;
	long	aaddr;
	short	stuff2[5];
	short	ireg;
} frame;
{
	register thread_t *tc = T_CURRENT;

	printf("IN KTRAP\n");
	ASSERT(tc != NULLTHREAD);

	switch(number) {
	    case 2:
	    case 3:
		printf("kernel ");
		printf("address error:  (pc=%x,usp=%x,a=%x,i=%x,f=%x)\n",
		       frame.pc, tc->t_usp, frame.aaddr, frame.ireg,
		       frame.fcode);
		prtargs();
		panic("I quit\n");
		break;

	    case 4:
		printf("kernel ");
		panic("illegal instruction:  (pc=%x,*pc=%x,sp=%x)\n",
			frame.pc, *(short *)(frame.pc), tc->t_usp);
		prtargs();
		break;

	    case 9:
		printf("kernel ");
		printf("trace:  (pc=%x,*pc=%x,sp=%x)",
			frame.pc, *(short *)(frame.pc), tc->t_usp);
		if (tc->t_watch != NULL)
			printf(" *w=%x\n", *(int *)(tc->t_watch));
		else
			printf("\n");
		break;

	    default:
		panic("unknown kernel trap type %d  (pc=%x,sp=%x)",
		      number, frame.pc, tc->t_usp);
		break;
	}

	ASSERT(tc->t_state & T_RUNABLE);
	debug("kernel trap returning to %x\n", frame.pc);
}


prtargs()
{
	printf("a0 - a6: %x %x %x %x %x %x %x\n",
		T_args[R_A0], T_args[R_A1], T_args[R_A2], T_args[R_A3],
		T_args[R_A4], T_args[R_A5], T_args[R_A6]);
	printf("d0 - d7: %x %x %x %x %x %x %x %x\n",
		T_args[R_D0], T_args[R_D1], T_args[R_D2], T_args[R_D3],
		T_args[R_D4], T_args[R_D5], T_args[R_D6], T_args[R_D7]);

	prtrace((int *)T_args[R_A6]);
}


prtrace(a6)
int	*a6;
{
	int	i = 10;
	int	*oa6;
	extern	int	kstack, kstop;

	if(a6 > (int *)STACKBASE && a6 < (int *)STACKTOP) {
		printf("user stack backtrace:\n");
		oa6 = (int *)STACKBASE;
		while(a6 > oa6 && a6 < (int *)STACKTOP && !((int)a6 & 1) && --i) {
			printf("a6: %x  ", a6);
			printf("retpc; %x  ", a6[1]);
			printf("*a6: %x\n", a6[0]);
			oa6 = a6;
			a6 = (int *)*a6;
		}
	}
	else if(a6 >= &kstack && a6 < &kstop) {
		printf("kernel stack backtrace op=%x p[0]=%x:\n",
			T_CURRENT->t_msg.m_opcode, T_CURRENT->t_msg.m_param[0]);
		oa6 = &kstack;
		while(a6 > oa6 && a6 < &kstop && !((int)a6 & 1) && --i) {
			printf("a6: %x  ", a6);
			printf("retpc; %x  ", a6[1]);
			printf("*a6: %x\n", a6[0]);
			oa6 = a6;
			a6 = (int *)*a6;
		}
	}
	else
		printf("invalid stack a6=%x\n", a6);
}
