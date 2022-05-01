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


/* interrupt definitions for sdugram stuff */

#define MBTO		(0 + 0)		/* multi bus time out */
#define NBTO		(0 + 1)		/* Nu bus time out */
#define TAPE_EINT	(0 + 2)		/* tape exception */
#define TAPE_RINT	(0 + 3)		/* ??? */
#define AC_INT		(0 + 4)		/* ac intrupt */
#define ETROC_INT	(0 + 5)		/* ?? */

#define PIC_2_INT	(0 + 6)		/* counter */
#define PIC_1_INT	(0 + 7)		/* multi bus time out */

#define PCI_0_RINT	(8 + 0)		/* pci 0 receive int */
#define PCI_0_TINT	(8 + 1)		/* pci 0 trans int */
#define PCI_1_RINT	(8 + 2)		/* pci 1 receive int */
#define PCI_1_TINT	(8 + 3)		/* pci 1 trans int */
#define	PIT_1_C0INT	(8 + 4)		/* timer int */
#define	CLK_INT		(8 + 4)		/* timer int */
#define	PIT_1_C1INT	(8 + 5)		/* timer int */
#define	PIT_1_C2INT	(8 + 6)		/* timer int */

#define	DISK_INT	(16 + 4)	/* smd disk int */
#define ETHER_INT	(16 + 0)	/* ethernet int */
#define CYPHER_INT	(16 + 2)	/* half inch tape */
