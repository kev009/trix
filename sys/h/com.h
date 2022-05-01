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


/*
 * The Trix communication mechanism is built upon a pair of objects:
 * a "port" represents an entry-point into a domain and a "handle" is
 * a protected pointer to a port.  Each port keeps a count of the
 * number of handles which refer to it, when this count becomes zero
 * the port is freed.
 */


struct	port	{
	short		p_state;	/* state of port */
#define		P_FREE		0
#define		P_USER		1
#define		P_KERNEL	2
#define		P_CLOSED	4
	short		p_handles;	/* count of handles on port */
	struct	link	p_link;		/* link in domain port list */
	struct	subdom	*p_subdom;	/* pointer to handler domain */
	funp_t		p_entry;	/* port handler function addr */
	arg_t		p_passport;	/* passport identification */
	short		p_priority;	/* port thread priority */
};

struct	port	ports[];


struct	handle	{
/*	struct	link	*h_link;	/* next handle in list of domain */
	struct	port	*h_port;	/* pointer to object port */
	struct	subdom	*h_subdom;	/* pointer to owner-domain */
};

struct	handle	handles[];
