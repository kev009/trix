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


typedef	unsigned char	stat_t;

#define	STATVAL(c,o,l)	(((l)+4) | ((o)<<8) | ((c)<<16))

#define	STATINIT(v)	((v) & 0377),		\
			(((v)>>8) & 0377),	\
			(((v)>>16) & 0377),	\
			(((v)>>24) & 0377)

#define	STATLONG(s)	(((s)[0]) | ((s)[1]<<8) | ((s)[2]<<16) | ((s)[3]<<24))

#define	LONGSTAT(s,v)	( ((s)[0] = ((v) & 0377)),		\
			  ((s)[1] = (((v)>>8) & 0377)),	\
			  ((s)[2] = (((v)>>16) & 0377)),	\
			  ((s)[3] = (((v)>>24) & 0377))	\
			)


/* portions of the status that may be preempted by filters */
#define	STATUS_MAXWIN	STATVAL(1,1,sizeof(long))	/* maximum window size */
#define	STATUS_MINWIN	STATVAL(1,2,sizeof(long))	/* minimum window size */
#define	STATUS_BANDW	STATVAL(1,3,sizeof(long))	/* bandwidth of path */

/* portions of the status that are dealt with by the object */
#define	STATUS_OWNER	STATVAL(2,1,sizeof(long))	/* ID of owner of object */
#define	STATUS_CTIME	STATVAL(2,2,sizeof(long))	/* time file created (GMT) */
#define	STATUS_MTIME	STATVAL(2,3,sizeof(long))	/* time file modified (GMT) */
#define	STATUS_ATIME	STATVAL(2,4,sizeof(long))	/* time file accessed (GMT) */
#define	STATUS_SUSES	STATVAL(2,5,sizeof(long))	/* count of "stable" uses */
#define	STATUS_AUSES	STATVAL(2,6,sizeof(long))	/* count of "active" uses */
#define	STATUS_ISIZE	STATVAL(2,7,sizeof(long))	/* available data */
#define	STATUS_OSIZE	STATVAL(2,8,sizeof(long))	/* available space */
#define	STATUS_IDENT	STATVAL(2,9,sizeof(long))	/* "identity" of object */
#define	STATUS_ACCESS	STATVAL(2,10,sizeof(long))	/* access mode of object */
#define	STATUS_PROT	STATVAL(2,11,sizeof(long))	/* protocol of object */
#define	STATUS_DOMAIN	STATVAL(2,12,sizeof(long))	/* machine/domain  handler */

/* tty status */
#define	STATUS_TTYIM	STATVAL(3,1,sizeof(long))	/* sysV input mode */
#define	STATUS_TTYOM	STATVAL(3,2,sizeof(long))	/* sysV output mode */
#define	STATUS_TTYCM	STATVAL(3,3,sizeof(long))	/* sysV control mode */
#define	STATUS_TTYLM	STATVAL(3,4,sizeof(long))	/* sysV local mode */
#define	STATUS_TTYFLG	STATVAL(3,5,sizeof(long))	/* v7 mode flags */
#define	STATUS_TTYPUSH	STATVAL(3,6,sizeof(long))	/* push window state */
#define	STATUS_TTYPOP	STATVAL(3,7,sizeof(long))	/* pop the window state */

/* net status */
#define	STATUS_IPADDR	STATVAL(4,1,6)			/* get net address */
#define	STATUS_ERRORS	STATVAL(4,2,sizeof(long))	/* lost packets */

/* chaos status calls */
#define	STATUS_CH_EOF	STATVAL(5,1,sizeof(long))
#define	STATUS_CH_SFLUSH	STATVAL(5,2,sizeof(long))
#define	STATUS_CH_TWAIT	STATVAL(5,3,sizeof(long))
#define	STATUS_CH_RECORD	STATVAL(5,4,sizeof(long))
