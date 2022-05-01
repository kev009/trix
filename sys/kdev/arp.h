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
 * Address resolution definitions - ethernet and chaosnet specific
 *
 *	NOTE! Bytes swapped around for the 68000!
 */


#define u_char char

struct	ar_packet	{
        short   ar_hardware;    /* Hardware type */
        short   ar_protocol;    /* Protocol-id - same as Ethernet packet type */
        u_char  ar_hlength;     /* Hardware address length = 6 for ethernet */
        u_char  ar_plength;     /* Protocol address length = 2 for chaosnet */
        short   ar_opcode;      /* Address resolution op-code */
        u_char  ar_esender[6];  /* Ethernet sender address */
	chaddr	ar_csender;	/* Chaos sender address */
        u_char  ar_etarget[6];  /* Target ethernet address */
        chaddr  ar_ctarget;     /* target chaos address */
};


/* Values for ar_hardware */

#define AR_ETHERNET     1	/* Ethernet hardware */


/* Values for ar_opcode */

#define AR_REQUEST      1	/* Request for resolution */
#define AR_REPLY        2	/* Reply to request */


/* Packet types */

#define	PUP_PUPTYPE	0x0400		/* PUP protocol */
#define	PUP_IPTYPE	0x0800		/* IP protocol */
#define	CHAOS_TYPE	0x0804		/* CHAOS protocol */
#define	ADDR_TYPE	0x0806		/* Address resolution (a la DCP) */


/* The ethernet broadcase address */

#define	ETHER_BROADCAST		{-1, -1, -1, -1, -1, -1}
