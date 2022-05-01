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


#define	E_FAILED	20		/* should go in args.h or com.h */

/*
 * Address resolution definitions - ethernet and chaosnet specific
 *
 *	NOTE! Bytes swapped around for the 68000!
 */

#define u_char char

struct	ethhdr	{
	short	eth_dest[3];		/* destination */
	short	eth_src[3];		/* src */
	short	eth_proto;		/* outer level protocol */
};


struct	ar_packet	{
	struct	ethhdr	er_ehdr;	/* for spacing */
        short   ar_hardware;		/* Hardware type */
        short   ar_protocol;    	/*  same as Ethernet packet type */
        u_char  ar_hlength;		/* address length = 6 for ethernet */
        u_char  ar_plength;		/* Protocol addrlength = 2 for chaosnet */
        short   ar_opcode;		/* Address resolution op-code */
        short   ar_esender[3];		/* Ethernet sender address */
	short	ar_csender;		/* Chaos sender address */
        short   ar_etarget[3];		/* Target ethernet address */
        short   ar_ctarget;		/* target chaos address */
};


/* Values for ar_hardware */

#define AR_ETHERNET     1		/* Ethernet hardware */


/* Values for ar_opcode */

#define AR_REQUEST      1		/* Request for resolution */
#define AR_REPLY        2		/* Reply to request */


/* Packet types */

#define	PUP_PUPTYPE	0x0400		/* PUP protocol */
#define	PUP_IPTYPE	0x0800		/* IP protocol */
#define	CHAOS_TYPE	0x0804		/* CHAOS protocol */
#define	ADDR_TYPE	0x0806		/* Address resolution (DCP) */

#define EMINSIZ		(70 +sizeof(struct ethhdr))	/* min packet size */
#define PKTSIZE		1200			/* make ethernet packet size */
#define NPACKS	4

struct	packet	{
	short	pk_flags;
	short	pk_dsize;		/* length of data (no hdr) in byte */
	struct	packet	*pk_forw;	/* forward pointer */
	struct	packet	*pk_back;	/* backward pointer */
	struct	ethhdr	pk_ehdr;	/* the ethernet hdr */
	char	pk_data[PKTSIZE];
};

extern struct packet packets[];


#define PK_FREE 0
#define PK_ALLOC 0x1
#define PK_QUEUED 0x2

struct	conn	{
	short	cn_flags;		/* state flags for structure  */
	short	cn_proto;		/* the protocol for this connection */
	struct	packet	*cn_head;	/* head of packet list */
	struct	packet	*cn_tail;	/* tail of packet list */
};


#define CN_FREE	0			/* this structure is free */
#define	CN_ALLOC 0x1			/* this structure is allocated */
#define CN_RLOC 0x2			/* some one is sleeping on me */
#define CN_XLOC 0x4			/* some one is sleeping on me */
#define CN_BUSY 0x8			/* transmitter busy */

#define NCONNS 8
extern struct conn conns[];		/* storage allocation */
extern struct packet *pk_alloc();
