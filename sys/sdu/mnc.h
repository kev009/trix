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

/*	mnc.h	1.2	6/27/83	*/


#define MNC_SLOT	0xF		/* normal slot of mnc */

#define MNCMDATA	0L		/* board offset of multibus space */
#define MNCMIO		0x100000L	/* board offset of i/o space */
#define MNCMAP		0x104000L	/* board offset of multibus map */

#define MNC_MAPA	0x8000L		/* map address in multibus space */
#define MNC_NMAP	1024		/* number of entries in map */
#define MNC_MPEN	0x800000L	/* map enable bit in map words */

#define MNC_MDPT	0x50		/* multibus port number of mode */

#define MNC_MAP		0x1		/* enable map access, disable xlate */
#define MNC_INT		0x2		/* mnc interrupt bit */
#define MNC_ALTEN	0x4		/* alternate NuBus enable */
#define MNC_NURST	0x8		/* NuBus reset */


#define MNC_MP1		0x51		/* extension of mode port */

#define MNC1_ENBL	0x1		/* enable flag from config */
#define MNC1_LED	0x2		/* led flag from config */
#define MNC1_IB		0x4		/* interrupt b flag from config */
#define MNC1_IA		0x8		/* interrupt a flag from config */


#define MNC_CFG		0xFFDFFCL	/* board offset of config register */

#define MNC_INIT	0x1		/* board initialize */
#define MNC_ENBL	0x2		/* board enable */
#define MNC_LED		0x4		/* led control */
#define MNC_INTA	0x8		/* interrupt A */
#define MNC_INTB	0x10		/* interrupt B */
#define MNC_INTC	0x20		/* interrupt C */
 
