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
	Trix Mouse Driver
	This code is a mouse driver for Mouse Systems Inc.
	
	Input event comes in a group of five bytes:
	
		byte	contents
		
		1	1 0 0 0 0 L M R
		2	delta x 1
		3	delta y 1
		4	delta x 2
		5	delta y 2
	
	There are two delta x and delta y samples per mouse event.
	The mouse will not transmit until it has changed position.
*/

#define LEFT	0x4	/* mask for left button first byte */
#define CENTER	0x2	/* mask for center button first byte */
#define RIGHT	0x1	/* mask for right button first byte */



struct position
{
	int	xloc;		/* x position count */
	int	yloc;		/* y position count */
	int	button;		/* mouse buttons */
};


