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


/* static char sccs_id[] = "@(#)reg.h	1.8"; */

/* register offsets for the SDU board */

#define REGSEG		0x1C00
#define REGSIZE		0x200L
 
/* base address offsets for the registers on the SDU board */

/* writing a 0 turns the LED on, writing a 1 turns it off */
/* at startup ATTN_LED and SET_UP_LED are turned on */
/* if all self-tests pass then only SET_UP_LED is turned on */
/* if cmos ram (configuration data) is valid then all LEDs are off */
/* if a UNIX (or other) system is running the RUN_LED is turned on */
#define LEDS		0x80
#define RUN_LED		0x01	/* LED bit to turn on or off */
#define SET_UP_LED	0x02	/* LED bit to turn on or off */
#define ATTN_LED	0x04	/* LED bit to turn on or off */

#define SWITCH	0x84	/* lower four bits: 0-4 are given as f,e,d,b,7 resp. */
#define SLOT_ID	0x84	/* upper four bits */

/* configuration and status registers 0 and 1 */
/* NuBus timeouts should NOT be enabled without also enabling bus conversion */
#define CSR0	0x08C
#define ENA_MBUS 0x1
#define BUS_CNV	0x2	/* bit to enable bus conversions */
#define ENA_NBUS 0x2	/* another name for the bit above */
#define BUS_TO	0x4	/* bit to enable nubus timeouts -
				write 0 to this CSR0 bit to disable */

#define CLOCKBITS 0x18 /*two bits for nubus clock: normal, slow, fast, no clk*/
#define NORMALCLOCK	0x00
#define SLOWCLOCK	0x08
#define FASTCLOCK	0x10
#define NOCLOCK		0x18

/*two bits for margining voltage: normal, low, hi*/
#define NORMALVOLT	0x00
#define HIGHVOLT	0x20
#define LOWVOLT		0x40
#define VOLTBITS	(HIGHVOLT|LOWVOLT)

/* NEVER should have both bits above set at the same time. */

#define CSR1	0x088
#define TP_RDY	0x1		/* tape ready signal (read only) */
#define RST_BUS	0x1		/* reset bus (only one cycle) (write only) */
#define RST_NBUS 0x2		/* resets the board - "long" reset */
#define RST_MBUS 0x4
#define TP_RESET	0x8	/* reset the controller */
#define	TP_REQ		0x10	/* pass a request to controller */
#define TP_ONLINE	0x40	/* put it on-line for reads and writes */
#define TP_EX	0x80		/* tape exception signal */

/* streamer registers */
#define TP_CMD	0x1A0		/* segment offset of control and status reg */
#define TP_DATA	0x400		/* segment offset of data register */
#define TPDSIZ	512		/* size of tape data area */

/* defines for the two PITs (8253 Programmable Interval Timers) */
#define PIT_0	0x170
#define PIT_1	0x160
#define CLOCK	PIT_1	/* will choose counter 0 of this PIT for the clock */


/* defines for the MC146818 time-of-day chip */
#define TOD_ADDR 0x124
#define TOD_DATA 0x120

/* two serial ports - 8251A Programmable Communications Interface chips */
/* lower two bits always 0 */
#define PCI_0	0x150	/* data is PCI_0, control is PCI_0 + 4 */
#define PCI_1	0x158

/* bus timeout register - determines how soon to give up waiting for response*/
/* (look at CSR0) */
#define BTOREG	0x180	/* ff is fastest, 00 is slowest (for waiting) (use 0)*/

/* interrupt register write a bit at a time - read all 8 bits at once */
#define INTREG	0x1E0	/* bit addrs are 1E0,1E4,1E8,1EC,1F0,1F4,1F8,1FC */
#define INTBIT0 (INTREG+4*0)
#define INTBIT1 (INTREG+4*1)
#define INTBIT2 (INTREG+4*2)
#define INTBIT3 (INTREG+4*3)
#define INTBIT4 (INTREG+4*4)
#define INTBIT5 (INTREG+4*5)
#define INTBIT6 (INTREG+4*6)
#define INTBIT7 (INTREG+4*7)


/* three 8259A Programmable Interrupt Controllers */
#define PIC_0	0x1C0
#define PIC_1	0x1C8
#define PIC_2	0x1D0	/* (interrupt register goes to this PIC) */
#define PIC_M	PIC_0	/* master PIC */
