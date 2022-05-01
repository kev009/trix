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


page_fault(tc)
register thread_t  *tc;
{
	extern	int	cpu_type;

	if(cpu_type == 68010) {
		/* for now we simply try growing the stack */
		if(tc->t_except.ex_faddr < STACKTOP &&
		   tc->t_except.ex_faddr >= tc->t_usp - PAGESIZE) {
			/* the bad address is in the range of the stack */
debug("fault: faddr=%x usp=%x size=%x\n",
tc->t_except.ex_faddr,
tc->t_usp,
tc->t_smap.m_seg->s_size);
			s_expand(tc->t_smap.m_seg,
				 2*PAGESIZE +
				  (STACKTOP-tc->t_usp)-tc->t_smap.m_seg->s_size);

			/* force the segment to be remapped */
			T_MAPPED = NULLSEGMENT;

			return(1);
		}
	}

	printf("page_fault on address %x\n", tc->t_except.ex_faddr);
	return(0);
}
