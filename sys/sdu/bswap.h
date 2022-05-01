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

 
/*	bswap.h	1.2	6/27/83	*/


struct nulong
{
	char a, b, c, d;
};

struct nushort
{
	char a, b;
};

struct m68long
{
	char d, c, b, a;
};

struct m68short
{
	char b, a;
};

/* byte swap long words */
#define lswap(x,y) {\
	((struct nulong *)(x))->a = ((struct m68long *)(y))->a; \
	((struct nulong *)(x))->b = ((struct m68long *)(y))->b; \
	((struct nulong *)(x))->c = ((struct m68long *)(y))->c; \
	((struct nulong *)(x))->d = ((struct m68long *)(y))->d; \
}

/* byte swap short words */
#define sswap(x,y) {\
	((struct nushort *)(x))->a = ((struct m68short *)(y))->a; \
	((struct nushort *)(x))->b = ((struct m68short *)(y))->b; \
}
