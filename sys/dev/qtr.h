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

/*
* Definitions for Cipher Series 400 Streaming Tape Driver - "Quarterback"
*/


/*
*  tape parameters
*/
#define TPBLKSIZ	512	/* number of bytes per tape block */
#define NSTATS		6	/* number of status bytes */

/*
*  Time in seconds to wait for various commands before assuming
*  that an interrupt has been lost.
*/
#define TRESET		1		/* reset */
#define TRDSTAT		1		/* read status */
#define TRDSPEND	1		/* set up read status command */
#define TCMDACPT	1		/* accept command */
#define TRDSDONE	1		/* get ready after cmd load */
#define TCMD		180		/* arbitrary command */
#define TIO		10		/* io operation for next block */
#define TREWIND		180		/* time to rewind tape */

/*
*  states
*/
#define IDLE	0	/* idle / waiting for cmd accept interrupt */
#define CMDACPT	1	/* cmd accepted, waiting for done/start io intr */
#define XREAD	2	/* read command executing */
#define XWRITE	3	/* write command executing */
#define EXCEPT	4	/* exception received, issued read status cmd */
#define RDSACPT	5	/* waiting for first status byte */
#define RDSTAT	6	/* currently reading status bytes */
#define RDSDONE 7	/* rdstat done, awaiting interrupt */

/*
*  commands
*/
#define Q_NULL_CMD	0xF0		/* psuedo command - null cmd */
#define Q_GO_OFFLINE	0xF1		/* psuedo command - take offline */
#define Q_RESET		0xF2		/* psuedo command - reset */
#define Q_SELECT0		0x01		/* select drive 0 */
#define Q_BOT		0x21		/* position to beginning of tape */
#define Q_ERASE		0x22		/* erase entire tape */
#define Q_RETENSION	0x24		/* retension tape */
#define Q_WRITE		0x40		/* write data */
#define Q_WTMARK		0x60		/* write file mark */
#define Q_READ		0x80		/* read data */
#define Q_RDMARK		0xA0		/* read file mark */
#define Q_STATUS		0xC0		/* read status command */
 
/*
*  Masks to see if cmd left tape at bot.  See p21 of manual.
*/
#define	POSMASK		0xF0		/* get high nibble */
#define POSITION	0x20		/* position cmds are 0x2X */

/*
*  Exception status bytes' meaning-- see p. 32-4 of manual
*/
#define NOCAR	0xC000		/* no cartridge in place */
#define NODRI	0xF000		/* no drive present */
#define WRIPRO	0x9000		/* write-protected cartridge */
#define EOM	0x8800		/* end of media */
#define RWABRT	0x8488		/* read or write abort */
#define RDBBX	0x8400		/* read error, bad block xfer */
#define RDFBX	0x8600		/* read error, filler block xfer */
#define RDND	0x8680		/* read error, no data (manual lies)*/
#define RDEOM	0x8EA0		/* read error, no data and eom */
#define FMRD	0x8100		/* filemark read */
#define ILCMD	0x00C0		/* illegal command */
#define POR	0x0081		/* power on/reset */

/*
*  Masks to deal with don't care bits in summary bytes
*/
#define MASK0	0xFFFF	/* no don't care bits-- EOM and NODRI */
#define MASK1	0xEFFF	/* one don't care bit-- all RD errors and NOCAR */
#define MASK3	0xFF76	/* three don't care bits-- WRIPRO (manual lies) */
#define MASK3A	0xEFCF  /* three other don't care bits-- RDND (manual lies) */
#define MASK5	0x0FF7	/* five don't care bits-- ILCMD and POR */	
#define MASK6	0xE000	/* just look at top 3 bits */

/*
*  Misc.
*/
#define NQTR		1	/* number of qtr's defined */
#define QTUNIT(x)	(0)	/* only 1 unit for now (forever?) */
#define QTRTICK		(50)	/* unit of timing ( 1 second ) */
#define T_NOREWIND	04	/* minor number of non-rewinding device */
#define CMDP(b)		(b == &qtrcmdbuf)	/* cmd bfr? */


