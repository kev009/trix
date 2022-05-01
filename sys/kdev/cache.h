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


/* sizes of cache entries */
#define	BLKSHFT	10		/* log of minimum transfer size */
#define	BLKSIZE	(1<<BLKSHFT)	/* (minimum) transfer size */
#define	BLKMASK	(BLKSIZE-1)


#define	CACHES	10

/* state bits for cache entries */
#define	C_DIRTY	(1<<0)		/* entry needs to be written out */
#define	C_USBSY	(1<<1)		/* entry is locked for use */
#define	C_RDBSY	(1<<2)		/* entry is being read */
#define	C_WRBSY	(1<<3)		/* entry is being written */
#define	C_IOBSY	(C_RDBSY|C_WRBSY)
#define	C_UWAIT	(1<<4)		/* USBSY is being waited on */
#define	C_IWAIT	(1<<5)		/* IOBSY is being waited on */
#define C_ERROR (1<<6)		/* error on I/O operation */

#define	C_READ	(1<<10)		/* the data must be read */
#define	C_WRITE	(1<<11)		/* the data must be written */
#define	C_NOW	(1<<12)		/* the data can be flushed */


struct cache	{
	struct	link c_next;
	int	c_state;	/* state of entry */
	int	c_bno;		/* starting block number of cache entry */
	int	c_disk;		/* disk with which block is associated */
	int	c_tim;		/* info for fifo algorithm */
	int	c_length;	/* length in 512 byte blocks of entry */
	long	c_adr;		/* swapped address for dma board */
	char	*c_data;	/* pointer to long aligned block */
	/* statistical data */
	int	c_hits;		/* number of times current data has hit */
};

#define	NULLCACHE	((struct cache *)0)
