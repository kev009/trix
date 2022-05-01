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


/* static char sccs_id[] = "@(#)confram.h	1.2"; */

/* structure of information contained in the sdu's nonvolatile ram */

#define NUCARDS 16	/* number of possible nubus cards */
#define MCARDS 8	/* number of possible multibus cards */

#define NSWITCH 5
#define NSP	2
#define NDSKPART 10
#define MDATA	24

/* defines for "config" driver */
#define IDXBITS	 0x000F
#define CMDBITS	 0x0FF0
#define WRITEBIT 0x1000
/* ioctl requests */
#define VERSION	 0x0010
#define CRC	 0x0020
#define SHELL	 0x0030
#define NUAVAIL	 0x0040
#define MCARD	 0x0050
#define SP	 0x0060
#define DSKPART  0x0070
#define CLEAR	 (0x0080|WRITEBIT)
#define ALL	 0x0090

/* LAYOUT OF START OF THE 2K BYTES OF CMOS RAM (1E000-20000, EVERY 4TH BYTE) */
/* contains information on the actual configuration of the system */

/* IMPORTANT - first two bytes of cmos ram are reserved for the crc, */
/*	next is rest of the cmos_ram structure, and then the shell scripts */

struct cmos_ram
{
	unsigned short crc;	/* cyclic redundancy check */
	struct sh {
		short sh_off;	/* offset (phys bytes) to switch script */
		short sh_size;	/* size (in bytes) of the shell script */
	} sh[NSWITCH];

	short nuavail;		/* bit=0 if not used; lowest bit is slot 0 */

	/* info on the multibus card slots */
	struct mcard {
		char m_name[8];		/* null means not available */
		char m_interrupt;	/* interrupt (0-7) */
		char m_port;		/* multibus IO address */
		char m_data[MDATA]; 	/* misc. info - e.g. disk params */
	} m[MCARDS];

	/* info for the serial port driver - used for initialization */
	struct sp {	/* check hardware description for meaning of bits */
		char sp_mode;	/* e.g. 1 stop bit, 8 bits/char, 16X clock */
		char sp_cmd;	/* e.g. RTS, ER, RXE, DTR, TXEN */
		short sp_baud;
	} sp[NSP];

	unsigned char version;	/* layout version of this structure */

	/* logical disk partitions */
	struct dskpart {
		char	d_user[8];	/* e.g. "filedrv", "unix", "lisp" */
		char	d_unit;		/* unit/device number */
		long	d_offset;	/* byte offset */
		long	d_size;		/* size in bytes */
	} dskpart[NDSKPART];
} ;
