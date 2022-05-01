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


#include "../h/cbuf.h"

/*
 *	TRIX 1.0
 *	Circular Buffer Routines
 *
 *	ROUTINES:
 *		cb_init(cbp, base, size)
 *		cb_read(cbp, func, addr, size)
 *		cb_write(cbp, func, addr, size)
 */

/*
 * CB_INIT - initialize a circular buffer
 */
cb_init(cbp, base, size)
register struct	cbuf	*cbp;
register char	*base;
{
	cbp->cb_base = base;
	cbp->cb_in = base;
	cbp->cb_out = base;
	cbp->cb_end = &base[size];
	cbp->cb_ccnt = 0;
	cbp->cb_size = size;
}


/*
 * CB_READ - read 'size' bytes from circular buffer.
 */
cb_read(cbp, addr, size)
register struct	cbuf	*cbp;
register size;
{
	register remb;
	register char	*ocp;

	if (cb_used(cbp) <= 0 || size <= 0)
		return(0);
	size = min(size, cb_used(cbp));
	ocp = cbp->cb_out;
	remb = cbp->cb_end - ocp;
	if ((cbp->cb_in > ocp) || (remb >= size))
	{
		t_STORE(addr, ocp, size);
		ocp += size;
		if (ocp == cbp->cb_end)
			ocp = cbp->cb_base;
		goto rex;
	}
	t_STORE(addr, ocp, remb);
	addr += remb;
	remb = size - remb;
	t_STORE(addr, cbp->cb_base, remb);
	ocp = cbp->cb_base + remb;

rex:	cbp->cb_ccnt -= size;
	cbp->cb_out = ocp;
	return(size);
}

/*
 * CB_WRITE - write 'size' bytes into circular buffer.
 */
cb_write(cbp, addr, size)
register struct	cbuf	*cbp;
register size;
{
	register remb;
	register char	*icp;

	if (cb_free(cbp) <= 0 || size <= 0)
		return(0);
	size = min(size, cb_free(cbp));
	icp = cbp->cb_in;
	remb = cbp->cb_end - icp;
	if ((cbp->cb_out > icp) || (remb >= size))
	{
		t_FETCH(addr, icp, size);
		icp += size;
		if (icp == cbp->cb_end)
			icp = cbp->cb_base;
		goto wex;
	}
	t_FETCH(addr, icp, remb);
	addr += remb;
	remb = size - remb;
	t_FETCH(addr, cbp->cb_base, remb);
	icp = cbp->cb_base + remb;

wex:	cbp->cb_ccnt += size;
	cbp->cb_in = icp;
	return(size);
}
