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

/* this is included since there is no documantation on TI video
 *	--dg
 */
 
struct vcmem {
	char	vc_freg0;	/* function register, bits 0-7 */
	char	vc_freg1;	/* function register, bits 8-15 */
	short	:16;
	char	vc_mcr;		/* memory control register */
	char	:8;
	short	:16;
	long	vc_intr;	/* interrupt address */
	char	vc_stat0;	/* status register, bits 0-7 */
	char	vc_stat1;	/* status register, bits 8-15 */
	short	:16;
	short	vc_scntl;	/* serial port control register */
	short	:16;
	char	vc_sxmit;	/* serial port transmit register */
	char	:8;
	short	:16;
	char	vc_srcv;	/* serial port recieve register */
	char	:8;
	short	:16;
	short	:16;
	short	:16;
	short	:16;
	short	:16;
	short	:16;
	short	:16;
	short	:16;
	short	:16;
	short	:16;
	short	:16;
	char	vc_adata;	/* serial port A data */
	char	:8;
	short	:16;
	char	vc_acmd;	/* serial port A command */
	char	:8;
	short	:16;
	char	vc_bdata;	/* serial port B data */
	char	:8;
	short	:16;
	char	vc_bcmd;	/* serial port B command */
	char	:8;
	short	:16;
};

struct vc_slt {
	short	slt_addr;
	short	slt_unused;
};

/* bits in vc_freg0 -- function register */
#define VC_RESET	0x1	/* reset */
#define VC_BDEN		0x2	/* board enable */
#define VC_LED		0x4	/* LED bit - just turns on the LED */
#define VC_F0		0x8	/* F0 bit function code */
#define VC_F1		0x10	/* F1 bit */
#define VC_FMASK	0x18	/* function code bit mask */
#define	VCF_XOR		0x00	/* xor function */
#define VCF_OR		VC_F0	/* set bits function */
#define VCF_AND		VC_F1	/* clear bits function */
#define VCF_STORE	(VC_F0|VC_F1)	/* store function */

/* bits in vc_mcr -- memory control register */
#define VC_REF0		0x1	/* refresh cycles per horiz. line */
#define VC_REF1		0x2	/* */
#define VC_MBNK		0x4	/* memory bank select */
#define VC_COPY		0x8	/* enable copy bank A to bank B */
#define VC_XOR		0x10	/* 1 => reverse video */
#define VC_IE		0x20	/* 1 => enable interrupts */
#define VC_BUS		0x40	/* select NuBus A or NuBus B */

/* bits in vc_intr */

#define VC_INTBIT	0x01000000L	/* low order bits must be this */


/* bits in vc_stat0 -- status register */
#define	VC_VB		0x01	/* vertical blank */
#define VC_FSEL		0x02	/* field select */
#define VC_HB		0x04	/* horizontal blank */
#define VC_HS		0x08	/* horizontal select */
#define VC_VS		0x10	/* vertial sync */
#define VC_CS		0x20	/* composite sync */
#define VC_CB		0x40	/* composite blank (inverted) */
#define VC_VID		0x80	/* TTL video output */

/* bits in vc_stat1 */
#define VC_PE		0x01	/* serial port parity error */
#define VC_FE		0x02	/* serial port fram error */
#define VC_OE		0x04	/* serial port overrun error */
#define VC_THRE		0x08	/* xmit holding data empty */
#define VC_TRE		0x10	/* xmit holding empty */
#define VC_MPT		0x20	/* FIFO empty */
#define VC_FUL		0x40	/* FIFO full */


/* bits in vc_scntl */
#define VCS_TA		0x0100	/* baud rate selection bits */
#define VCS_TB		0x0200
#define VCS_TC		0x0400
#define VCS_TD		0x0800
#define VCS_SBS		0x1000	/* stop bit select, false=1 stop bit, true=2 */
#define VCS_EPE		0x2000	/* even parity enable, true = even */
#define VCS_WLS1	0x4000	/* word length select bits */
#define VCS_WLS2	0x8000
#define VCS_SPPI	0x0001	/* serial port parity inhibit */


#define VCS_BIT8	(VCS_WLS1 | VCS_WLS2)

/* ver 1 serial port or SIO channel A baud rate definitions */
#define VCS_A9600	(VCS_TD | VCS_TC | VCS_TB)
#define VCS_A2400	(VCS_TD | VCS_TB)
#define VCS_A1200	VCS_TD

/* SIO channel B baud rate definitions */
#define VCS_B9600	((VCS_TD | VCS_TC | VCS_TB) << 4)
#define VCS_B2400	((VCS_TD | VCS_TB) << 4)
#define VCS_B1200	(VCS_TD << 4)

/* SIO register select */
#define VC_R0	0x00
#define VC_R1	0x01
#define VC_R2	0x02
#define VC_R3	0x03
#define VC_R4	0x04
#define VC_R5	0x05
#define VC_CRST		0x18  /* channel reset */
#define VC_ERST		0x30  /* error reset */
#define VC_XRST		0x10	/* ext/status reset */
#define VC_EIRX		0x20	/* en int on next RX */
#define VC_RITX		0x28	/* rest Tx int */
#define VC_EOI		0x38	/* end of int */

/* SIO channel A register definitions for pegasus keyboard */
#define VC_AWR1		0x18  /* enable receive ints */
#define VC_AWR3		0xc1  /* 8 bits/char, enable receive */
#define VC_AWR4		0x87  /* x32 clock, even parity, 1 stop bit */
#define VC_AWR5		0xea  /* DTR, 8 bits/char, xmit enable, RTS */

/* SIO channel B register definitions for mouse */
#define VC_BWR1		0x18  /* enable receive interrupts */
#define VC_BWR3		0xc1  /* 8 bits/char, enable receive */
#define VC_BWR4		0x84  /* x32 clock, no parity, 1 stop bit */
#define VC_BWR5		0xea  /* DTR, 8 bits/char, xmit enable, RTS */

/* SIO read register 0 bit definitions */
#define VC_R0RCA	0x1   /* receive character available */
#define VC_R0TBE	0x4   /* xmit buffer empty */
#define VC_R0BRK	0x80  /* break */

/* SIO read register 0 bit definitions */
#define VC_R1PE		0x10  /* parity error */
#define VC_R1OE		0x20  /* overrun error */
#define VC_R1FE		0x40  /* framing error */

/* constants */
#define VC_SLTOFF	0x6000	/* SLT starts at 24k */
#define VC_SLTENT	2048	/* number of scan line table entries */
#define VC_SLTSIZ	(VC_SLTENT * sizeof(struct vc_slt))

#define VC_VLTOFF	0x2000	/* VLT starts at 8k */
#define VC_VLTENT	16	/* number of VLT entries */
#define VC_VLTSIZ	(VC_VLTENT * sizeof(long))

#define VC_RAMOFF	0x20000L	/* memory starts at 128k */
#define VC_RAMSIZ	131072L

#define VC_ROMOFF	0xfffc00L	/* offset of configuration ROM */


/* assumptions */
#define VC_XSIZE	800		/* pixels per line */
#define VC_YSIZE	1024		/* lines per display */
#define VC_LLEN		128		/* bytes per scan line */
	/* VC_LLEN used to be (VC_XSIZE/8), but the video card doesn't
	   handle that right. */
#define VC_RAMUSED	(VC_LLEN * VC_YSIZE)
					/* amount of vcmem ram used */
 
