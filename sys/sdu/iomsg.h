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
	(C) COPYRIGHT, TEXAS INSTRUMENTS INCORPORATED, 1983.  ALL
	RIGHTS RESERVED.  PROPERTY OF TEXAS INSTRUMENTS INCORPORATED.
	RESTRICTED RIGHTS - USE, DUPLICATION, OR DISCLOSURE IS SUBJECT
	TO RESTRICTIONS SET FORTH IN TI'S PROGRAM LICENSE AGREEMENT AND
	ASSOCIATED DOCUMENTATION.
*/

 
/*	iomsg.h	1.4	6/27/83	*/


/* message format for communcation with the SDU */
/* things are long aligned to help things like the lisp machine */

#define IO_DONE	0x80		/* done bit, notice bit reversal */

struct iomsg
{
	unsigned char done;
	unsigned char dummy;
	unsigned short channel;	/* io channel used on port */
	unsigned short fcode;	/* function code (see below) */
	unsigned short errcode;	/* error code if value is 0 */
	long cnt;		/* number of bytes to move */
	long value;		/* value or status code on completion */
	long offset;		/* byte offset for read and write commands */
#define vecnum offset		/* for "readability" */
	long buffer;		/* NuBus address of buffer */
#define intad buffer		/* for "readability" */
	long wakeup;		/* NuBus address of where to interrupt */
};


struct ioport
{

unsigned char busy;
#define IOP_BUSY	0x80
unsigned char valid;
#define IOP_VALID	0x80
unsigned short dummy;

	long msg;		/* NuBus address of msg block */
	long wakeup;		/* NuBus address of where to interrupt */
};




/* function codes for io messages */

#define IO_OPEN	1
#define IO_CLOSE 2
#define IO_READ 3
#define IO_WRITE 4
#define IO_CTL 5
#define IO_EXIT 6
#define IO_SREAD 7	/* scatter/gather read */
#define IO_SWRITE 8	/* scatter/gather write */
#define IO_CLOCK 9	/* start clock interrupting at an address */
#define IO_DISK 10	/* assign disk interrupts to cpu */
#define IO_MALLOC 11	/* multibus alloc */
#define IO_INTR 12	/* establish an interrupt handler */
#define IO_DATE 13	/* read the date into a timeb type structure */
#define	IO_REBOOT 14	/* return to bootstrap */

/* ioport ioctl codes */

#define IOMSG	('i' << 8 | 0)		/* get nubus ptr to io msg */

/* location of io port in multibus memory */
#define IOPORT	0x14L
 
