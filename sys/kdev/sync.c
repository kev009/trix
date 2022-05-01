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


struct	list	sleeplist;


hndl_t
sync_make()
{
	static	int	syncnum = 0;

	return(p_kalloc(sync_handler, ++syncnum, 0));
}


sync_handler(passport, mp)
register struct message *mp;
{
	/* THIS IS CALLED ON THE KERNEL STACK (as such its called at SPL_WAKEUP) */

	switch(mp->m_opcode) {
	    case SLEEP:
		ksleep(passport, (caddr_t)mp->m_param[0]);
		/* the return through trap will call schedule */
		/* if we care about the wakeup value being returned, return(1) */
		break;

	    case WAKEUP:
		debug("wakeup %d\n", mp->m_param[0]);
		kwakeup(passport, mp->m_param[0]);
		break;

	    case CLOSE:
	    {	register struct link  *l;
		register thread_t  *tp;

		debug("sync_handler: CLOSE %x\n", passport);
		FOREACHENTRY(l, sleeplist, t_wait.t_sleep.t_slnk, tp, thread_t *) {
			if(tp->t_wait.t_sleep.t_syncn == passport)
				t_wakeup(tp, CLOSE);
		}
		break;
	    }

	    default:
		mp->m_ercode = E_OPCODE;
		return(0);
	}

	/* everything is OK but DONT rerun this request */
	mp->m_ercode = E_OK;
	return(0);
}
