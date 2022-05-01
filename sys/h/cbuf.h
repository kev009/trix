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
 * circular buffer data structure
 */
struct cbuf
{
	char	*cb_base;	/* pointer to beginning of buffer	*/
	char	*cb_end;	/* pointer to end of buffer		*/
	char	*cb_in;		/* pointer for next input character	*/
	char	*cb_out;	/* pointer for next output character	*/
	short	cb_ccnt;	/* number of characters in buffer	*/
	short	cb_size;	/* size of buffer			*/
};

#define	cb_used(cbp)	((cbp)->cb_ccnt)
#define cb_free(cbp)	((cbp)->cb_size - (cbp)->cb_ccnt - 1)
#define cb_quart(cbp)	((cbp)->cb_size >> 2)

#define	min(x,y) ((x > y) ? y : x)
#define max(x,y) ((x > y) ? x : y)
