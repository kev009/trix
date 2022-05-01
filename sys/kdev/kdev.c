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
#include	"../h/global.h"
#include	"../h/link.h"
#include	"../h/com.h"
#include	"../h/memory.h"
#include	"../h/domain.h"
#include	"../h/thread.h"
#include	"../h/assert.h"


struct	list	sleeplist;


krerun()
{
	/* return <> 0 if this is a rerun of a KREQUEST */
	return(T_CURRENT->t_state & T_KREQUEST);
}


kkernel()
{
	/* return <> 0 if this thread is running in the kernel */
	return(T_CURRENT->t_state & T_KERNEL);
}


kstore(dwaddr, data, size)
register char	*dwaddr;
register char	*data;
register int	size;
{
	int	ret = size;

	ASSERT(dwaddr>=USERBASE&&dwaddr+size<DWTOP || SYSSUBDOM(T_CURRENT->t_subdom));

	mcopy(data, dwaddr, size);
/*	while(--size >= 0)
		*dwaddr++ = *data++;
*/	return(ret);
}


kfetch(dwaddr, data, size)
register char	*dwaddr;
register char	*data;
register int	size;
{
	int	ret = size;

	ASSERT(dwaddr>=USERBASE&&dwaddr+size<DWTOP || SYSSUBDOM(T_CURRENT->t_subdom));

	mcopy(dwaddr, data, size);
/*	while(--size >= 0)
		*data++ = *dwaddr++;
*/	return(ret);
}


ksleep(syncn, saddr)
caddr_t	saddr;
{
	register thread_t  *tp = T_CURRENT;

	debug("thread %x ksleeping on %d:%x\n", tp, syncn, saddr);

	tp->t_wait.t_sleep.t_syncn = syncn;
	tp->t_wait.t_sleep.t_saddr = saddr;
	tp->t_state &= ~T_RUNABLE;
	tp->t_state |= T_KSLEEPING;

	/* unschedule the thread leaving the domain active */
	t_unsched(tp);

	/* put thread in list of sleeping threads */
	l_link(&sleeplist, &tp->t_wait.t_sleep.t_slnk);
}


kwakeup(syncn, saddr)
caddr_t	saddr;
{
	register thread_t  *tp;
	register struct link  *l;

	FOREACHENTRY(l, sleeplist, t_wait.t_sleep.t_slnk, tp, thread_t *) {
		if(tp->t_wait.t_sleep.t_syncn == syncn &&
		   tp->t_wait.t_sleep.t_saddr == saddr)
			t_wakeup(tp, 0);
	}
}


kwakeval()
{
	return(T_CURRENT->t_wait.t_wakeup);
}
