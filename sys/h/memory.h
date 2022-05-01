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
 *  definition of a memory segment
 *  a segment is the basic unit of virtual memory control
 */

#define SSIZE	12		/* 3 Mb segments for now */
#define	SLENG	(SSIZE*262144)

struct	segment {
	short	s_mode;		/* mode of the segment */
	off_t	s_size;		/* size of segment in bytes*/
	hndl_t	s_hndl;		/* handle mapped into the segment */
	pte_t	s_L1map[SSIZE];	/* pointers to segmep page maps */
};

/* segment state bits */
#define	S_FREE	0
#define	S_ALLOC	(1<<0)		/* segment is allocated */
#define	S_SWAP	(1<<1)		/* stable copy of segment is swapped */
#define	S_MOVE	(1<<2)		/* segment is being swapped in or out */
#define	S_LOCK	(1<<3)		/* the segment is currently locked */
#define	S_WAIT	(1<<4)		/* a thread is sleeping on the lock */
#define	S_DIRTY	(1<<5)		/* segment is dirty - write back on free */
#define	S_STACK	(1<<6)		/* segment grows top down (like a stack) */
#define	S_ZERO	(1<<7)		/* segment is initialize to zero on access */
#define	S_READ	(1<<8)		/* read handle to initialize on access */


/*
 *  a map is a particular user view of a segment
 *  for now only entire segments are mapped in,
 *    later a partial map might be interesting
 */

struct	map	{
	struct	segment	*m_seg;
	caddr_t	m_addr;
};


/* clicks (1K bytes) to bytes and back */
#define	btoc(x)		((((unsigned long)(x)+1023) >> PGSHIFT))
#define	ctob(x)		((x) << PGSHIFT)

/* Bytes to pages without rounding and back */
#define btop(x)		((unsigned long)(x) >> PGSHIFT)
#define ptob(x)		((unsigned long)(x) << PGSHIFT)

/* Virtual address to Level 1 Map Index */
#define btol1(x)	(btop(x) >> 8)
/* Virtual address to Level 2 Map Index of page of pte's */
#define	btol2(x)	(btop(x) & 0xFF)


/* flags for maps */

#define	CP_FREE		0
#define	CP_ALLOC	1
#define	CP_LOCKED	2

#define CMAPSIZE 3072
int	cmapsize;
char	coremap[CMAPSIZE];


/* status bits for the pte's in the TI memory management hardware */

#define PG_KR	0x00
#define PG_KW	0x10
#define PG_UR	0x20		/* full kernel access */
#define PG_UW	0x30

#define PG_M	0x1		/* modified */
#define PG_A	0x2		/* accessed */
#define PG_V	0x4		/* valid */
#define PG_C	0x8		/* cached */

/* with a single processor we can cache everything */
#define	PG_RTMP		(PG_UR | PG_V)
#define	PG_WTMP		(PG_UW | PG_C | PG_V)
#define	PG_UTXT		(PG_UR | PG_C | PG_V)
#define	PG_UDAT		(PG_UW | PG_C | PG_V)
#define	PG_USTK		(PG_UW | PG_C | PG_V)

#define	NEWMAP(m,p,f)	(*(long *)(m) = (long)(p) | (long)(f) | PG_V)
