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

/* This stuff is used because the SDU uses it to init the
 * cpu board. This is how the CPU finds out where memory is for
 * now. Perhaps later we will have it size memory and dump this stuff
 *	--dg
 */
 
/*	Device map structure passed to Unix on startup at top of stack */

#define MAXCPU		4	/* max number of cpu's (optimistic aren't I?)*/
#define MAXVCMEM	4	/* maximum vcmem boards that unix can handle */
#define MAXRAM		16	/* maximum ram boards that unix can handle */
#define MAXPORT		16	/* communication ports in NuBus space */
#define MAXDISK		8	/* max number of logical disks supported */
#define MAXCARDS	8	/* max number of MultiBus cards */
#define MAXDATA		24	/* # of data bytes per MultiBus card */

/* Ram is assumed to allocated to Unix in contiguous hunks of pages */
struct ramhunk
{
	long ramaddr;	/* NuBus address of ram */
	long ramsize;	/* size of ram in 1k byte pages */
};

struct devmap
{
	long cpu[MAXCPU];	/* cpu slot address */
	long vcmem[MAXVCMEM];	/* slot addresses of vcmem boards assigned */
	struct ramhunk ram[MAXRAM];	/* hunks of ram */
	struct
	{
		long ioport;	/* NuBus address of ioport */
		long wakeup;	/* NuBus address of wakeup interrupt */
	} port[MAXPORT];
	struct mbcard
	{
		char name[8];		/* null means not available */
		char interrupt;		/* interupt (0-7) */
		char port;		/* MultiBus IO address */
		char data[MAXDATA];	/* misc info - e.g. disk params */
	} mbcard[MAXCARDS];
	struct
	{
		char user[8];	/* e.g. "root", "usr" */
		char unit;	/* unit/device number */
		short port;	/* port number to use */
		short channel;	/* channel on port */
		long size;	/* length of disk in bytes */
		long offset;	/* logical offset in bytes */
	} disk[MAXDISK];
	short logport;		/* port to use for messages log */
	short logchannel;	/* channel on port to use */
	char console[8];	/* name of unix console device */
};

#define ROOTDISK	0	/* in disk table, the root disk */
#define SWAPDISK	1	/* in disk table, the swapping disk */
#define PIPEDISK	0	/* in disk table, the disk pipes come from */
#define WINDDISK	0	/* in disk table, the window disk */

#define SDUPORT		0	/* in port table, this one talks to sdu */


struct devmap devmap;
struct devmap *initmap;

 

