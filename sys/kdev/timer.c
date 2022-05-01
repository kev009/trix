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


#include "../h/param.h"
#include "../h/args.h"
#include "../h/calls.h"
#include "../h/link.h"


#define	NULLP	0
#define NCALL	20

struct	outcall	{
	struct	link	c_link;
	int	(*c_func)();		/* call function */
	int	c_arg;			/* call argument */
	int	c_time;			/* call time */
};

static	struct	outcall	clkcalls[NCALL];
struct	list	calllist;

extern	long	trixtime;

struct	timer	{
	int	tm_flags;
	struct	list	tm_list;	/* list of sleeping threads */
};


/* there is one trq structure for every thread waiting */

struct	trq	{
	int	tq_flags;
	int	tq_time;		/* number of ticks to go */
	caddr_t	tq_saddr;		/* address sleeping on */
	struct	link	tq_wlink;	/* linked list for wakeups */
	struct	link	tq_tlink;	/* link for timeout */
};


struct list timlist;			/* list of trq structures */


#define NTIMER	20
#define NTRQ	20

#define OB_ALLOC  1
#define OB_FREE 0
struct	timer	timers[NTIMER];
struct	trq	trqs[NTRQ];


struct	timer	*
tm_alloc()
{
	int	i;

	for(i = 0 ; i < NTIMER ; i++) {
		if(!(timers[i].tm_flags & OB_ALLOC)) {
			timers[i].tm_flags |= OB_ALLOC;
			return(&timers[i]);
		}
	}
	return((struct timer *)NULL);
}


tm_free(tp)
struct timer *tp;
{
	tp->tm_flags = OB_FREE;
}


struct trq *
tq_alloc()
{
	int	i;

	for(i = 0 ; i < NTRQ ; i++) {
		if(!(trqs[i].tq_flags & OB_ALLOC)) {
			trqs[i].tq_flags |= OB_ALLOC;
			return(&trqs[i]);
		}
	}
	return((struct trq *)NULL);
}


tq_free(tp)
struct	trq	*tp;
{
	tp->tq_flags = OB_FREE;
}


tim_handler(tp, mp)
struct	message	*mp;
struct	timer	*tp;
{
	register struct	trq	*trqp;
	register struct	link	*l;

	/* THIS IS CALLED ON THE KERNEL STACK (as such its called at SPL_WAKEUP) */

	switch(mp->m_opcode) {
	    case SLEEP:
		if(!krerun()) {
			if((trqp = tq_alloc()) == (struct trq *)NULL) {
				printf("can't allocate timer\n");
				mp->m_ercode = E_NOBJECT;
				return(0);
				break;
			}
			trqp->tq_saddr = (caddr_t)mp->m_param[0];
			trqp->tq_time = (int)mp->m_param[1];
			/* link into lists */
			l_link(&tp->tm_list, &trqp->tq_wlink);
			l_link(&timlist, &trqp->tq_tlink);
			ksleep(0, trqp);
			/* rerun the request */
			return(1);
		}
		else
			mp->m_param[0] = trixtime;
		break;

	    case WAKEUP:
		/* wakeups must do unlink else close won't work */
		FOREACHENTRY(l, tp->tm_list, tq_wlink, trqp, struct trq *) {
			if(trqp->tq_saddr ==(caddr_t)mp->m_param[0]) {
				l_unlink(&trqp->tq_wlink);
				l_unlink(&trqp->tq_tlink);
				kwakeup(0, trqp);
				tq_free(trqp);
			}
		}
		break;

	    case CLOSE:
		/* wake everyone up and free all storage */
		FOREACHENTRY(l, tp->tm_list, tq_wlink, trqp, struct trq *) {
			l_unlink(&trqp->tq_wlink);
			l_unlink(&trqp->tq_tlink);
			kwakeup(0, trqp);
			tq_free(trqp);
		}
		tm_free(tp);
		break;

	    case CONNECT:
		mp->m_ercode = E_ASIS;
		return(0);
		break;

	    default:
		mp->m_ercode = E_OPCODE;
		return(0);
	}

	/* DONT rerun the request */
	mp->m_ercode = E_OK;
	return(0);
}


hndl_t
tim_make()
{
	struct	timer	*tp;
	hndl_t	hh;

	if((tp = tm_alloc()) == (struct timer *)NULL)
		return(NULLHNDL);

	l_list(&tp->tm_list);
	if((hh = p_kalloc(tim_handler, tp, 0)) == NULLHNDL)
		tm_free(tp);
	return(hh);
}


/* tim_int called at SPL_WAKEUP, decrement counts and do wakeups */

tim_int()
{
	struct	link	*l;
	struct	trq	*trqp;
	struct outcall *ocp;

	if((timlist.next == NULLLINK) && (calllist.next == NULLLINK))
		return;

	FOREACHENTRY(l, timlist, tq_tlink, trqp, struct trq *) {
		if(--(trqp->tq_time) <= 0) {
			l_unlink(&trqp->tq_wlink);
			l_unlink(&trqp->tq_tlink);
			kwakeup(0, trqp);
			tq_free(trqp);
		}
	}
	
	FOREACHENTRY(l, calllist, c_link, ocp, struct outcall *) {
		if(ocp->c_func && (ocp->c_time > 0)) {
			if(--ocp->c_time == 0){
				ocp->c_func(ocp->c_arg);
				ocp->c_func = 0;
				l_unlink(&ocp->c_link);
				continue;
			}
		}
		else{
			printf("Bad callout in list\n");
			l_unlink(&ocp->c_link);
		}
	}
}


/*
 *  have the clock perform an outcall of the form:
 *    func(arg) to be called in 'time' clock ticks
 */

struct	outcall	*
clk_call(func, arg, time)
int	 (*func)();
{
	register struct	outcall	*ocp;
	register s;

	s = splx(SPL_WAKEUP);		/* low priority clock function */
	for(ocp = &clkcalls[0] ; ocp < &clkcalls[NCALL] ; ocp++) {
		if(ocp->c_func == 0)
				goto found;
	}
	panic("clkcall table overflow");

found:
	ocp->c_func = func;
	ocp->c_arg = arg;
	ocp->c_time = time;
	l_inlink(&calllist,&ocp->c_link);
	splx(s);

	return(ocp);
}		
