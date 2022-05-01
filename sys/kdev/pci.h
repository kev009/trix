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


/* defines for mode word */
#define	PCI_SYNC	0		/* sync mode */
#define	PCI_1X		0x1		/* 1x clock */
#define PCI_16X		0x2		/* 16x clock */
#define	PCI_64X		0x3		/* 64x cloxk */

#define PCI_8BIT	0xC		/* 8 bit characters */
#define PCI_PAR		0x10		/* parity enable */
#define PCI_EVEN	0x20		/* even parity if on */
#define PCI_1BIT	0x40		/* one stop bit */
#define PCI_2BIT	0xC0		/* TWO stop bits */

/* defines for command word */
#define	PCI_TXEN	0x1		/* transmit enable */
#define PCI_DTR		0x2		/* dtr pin */
#define PCI_RXEN	0x4		/* receive enable */
#define PCI_BRK		0x8		/* send break (txdat = 0) */
#define PCI_ERST	0x10		/* error reset */
#define PCI_RTS		0x20		/* RTS pin */
#define PCI_RST		0x40		/* reset 8251 */
#define PCI_HUNT	0x80		/* hunt mode (sync mode only */

/* defines for status word */
#define PCI_TXRDY	0x1		/* transmit ready */
#define PCI_RXRDY	0x2		/* receive ready (empty) */
#define PCI_TXEMP	0x4		/* tx empty */
#define PCI_PE		0x8		/* parity error */
#define PCI_OE		0x10		/* overrun error */
#define PCI_FE		0x20		/* framing error */
#define PCI_BDT		0x40		/* break detect */
#define PCI_DSR		0x80		/* dsr */

#define PIT_0 0x170			/* baud rate time offset */

extern	char asy[];


struct	pci	{
	char	data;
	char	:8;
	short	:16;
	char	stat;
	char	:8;
	short	:16;
};


struct	I8253	{
	struct	{
		char	pit_cnt;
		char	:8;
		short	:16;
	}   p_pit[3];
	char	pit_cmd;
	char	:8;
	short	:16;
};
