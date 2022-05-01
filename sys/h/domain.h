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
 *  a domain is the address space of resources (memory and handles) in
 *    which threads run
 */


struct	subdom	{
	struct	list	d_hlist;	/* accessible handles */
	struct	list	d_plist;	/* ports that are active */
	struct	domain	*d_domain;	/* supdomain */
};


struct	domain	{
	int	d_state;		/* state of domain */
#define		D_FREE		0
#define		D_ALLOC		(1<<0)		/* domain is allocated */
#define		D_MARK		(1<<1)		/* garbage collection mark bit */
#define		D_CLOSE		(1<<2)		/* close thread is currently in use */
#define		D_DEAD		(1<<3)		/* no close threads in dead domain */
#define		D_SYS		(1<<4)		/* the domain is a system domain */
	time_t	d_time;			/* total thread time in domain */
	hndl_t	d_handle;		/* handle for spawns and GO's */
 
	/* description of the address space -- should be an array of maps */
	struct	map	d_tdmap;

	/* used to control execution of threads in the domain */
	short	d_priority;		/* current priority level */
	struct	list	d_schdlst;	/* threads affecting priority */
	struct	list	d_access;	/* other threads */

	struct	list	d_pdead;	/* ports waiting to be closed */

	struct	subdom	d_subdom;	/* primary subdomain of domain */

	long	d_uid;			/* user id of the domain */
};

struct	domain	domains[];


#define	SYSDOMAIN(d)	((d)->d_state & D_SYS)
#define	SYSSUBDOM(d)	SYSDOMAIN((d)->d_domain)
