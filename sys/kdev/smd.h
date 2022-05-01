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

#ifdef SCCSID
static char id_smd[] = "@(#)smd.h	1.4 (Texas Instruments) 83/06/30";
#endif
 
#ifndef SCCSID
 
typedef unsigned char BYTE;
typedef unsigned short WORD;

#define DISKNAME "disk"

/* controller registers */

#define SMD	0x40		/* base IO address - hardware dependent */
#define RCMD	(SMD+0)		/* command register (only writable register) */
#define RSTATUS	(SMD+0)		/* status register */
#define RXMB	(SMD+1)		/* bits 24-16 of the iopb address */
#define RMSB	(SMD+2)		/* bits  15-8 of the iopb address */
#define RLSB	(SMD+3)		/* bits   7-0 of the iopb address */

/* possible values to write to the RCMD register - may be 'or'ed */
#define GO_CMD		1
#define CLEARINT	2
#define CLEAR_GO	(GO_CMD|CLEARINT)
#define BUSW		0x20	/* 0 for byte mode, 1 for word mode */

/* status bits for RSTATUS */

#define BUSY	0x01
#define INTPEND	0x02		/* operation has been completed */
#define U1RDY	0x10
#define U2RDY	0x20
#define U3RDY	0x40
#define U4RDY	0x80

struct iopb {
	BYTE	comm;		/* command */
	BYTE	options;	/* command options */
	BYTE	stat;		/* set to 0 before doing each cmd */
	BYTE	error;		/* error code/number retries */
	BYTE	unit;		/* unit select (0-3) */
	BYTE	head;           /* head select */
	WORD	cyl;		/* cyl no. */
	WORD	sect;   	/* sector no. */
	WORD	scnt;   	/* no. sectors to r/w */
	unsigned dmacnt:8;	/* busburst lngth (1-256 - 16 common)*/
	unsigned bfraddr:24;	/* memory address of buffer */
	WORD	ioa;		/* io address (set equal to SMD) */
	WORD	seg;		/* memory seg rel. addr */
	unsigned i_reserved:8;	/* (unused byte) */
	unsigned next:24;	/* linked iopb address */
} ;

struct uib {
	BYTE	ntracks;	/* number of heads/unit */
	BYTE	spertrk;	/* sectors/track */
        BYTE    bpslow;         /* bytes/sector (low byte) */
        BYTE    bpshi;          /* bytes/sector (high byte) */
	BYTE	gap1;   	/* bytes in gap 1 */
	BYTE	gap2;		/* bytes in gap 2 */
	BYTE	interlv;	/* interleave factor */
	BYTE	retries;	/* retry count */
	BYTE	ecc_enab;	/* error correction enable (0 or 1) */
	BYTE	rsk_enab;	/* reseek enable (0 or 1) */
	BYTE	mbd_enab;	/* move bad data enable (0 or 1) */
	BYTE	ibh_enab;	/* increment by head enable (0 or 1) */
	BYTE	dualport;	/* dual port drive (0 or 1) */
	BYTE	int_change;	/*interrupt on status change (0 or 1)*/
	BYTE	spiral;		/* spiral skew factor */
	BYTE	u_res1;		/* reserved byte 1 */
	BYTE	u_res2;		/* reserved byte 2 */
} ;

/* commands for comm field in iopb */

#define READ_CMD  0x81
#define WRITE_CMD 0x82
#define VERIFY	0x83
#define TRKFORMAT 0x84
#define MAP	0x85
#define	INITIALIZE 0x87
#define RESTORE	0x89
#define SEEK	0x8A
#define ZEROSEC	0x8B
#define RESET	0x8F

/* command options */
#define LINKIOPB 0x80
#define ABSADDR	0	/* (choose ABSADDR or RELADDR) */
#define RELADDR	0x20
#define BUSWIOPB 0x10	/* 0 for byte mode, 1 for word mode */
#define RSRV	0x8	/* if set, dual port drive is RESERVED after cmd done*/
#define	ABSBUF	0	/* (choose ABSBUF or RELBUF) */
#define	RELBUF	0x2
#define BUSWBUF	0x1	/* 0 for byte mode, 1 for word mode */


/* status codes for the stat field in iopb (returned by controller) */

#define CMDOK	0x80	/* command successul, ready for next command */
#define CMDBUSY	0x81	/* command in progress, busy */
#define CMDERR	0x82	/* command completed with an error */

/* error codes (if stat field in iopb set to CMDERR) */
/* (not used yet) */
#define EC	0x80	/* set if error correction has been applied */
#define BD	0x40	/* set if a restore and reseek sequence was done */
#define RS	0x10	/* set if bad data was moved into memory (see uib) */
#define RTRYMASK 0xF	/* mask to use to get number of rotational retries */

/* for dmacnt field in iopb */
#define NBURST	128

#endif

