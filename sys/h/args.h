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
 * location of registers saved in T_args[R_XX]
 */


#define	R_D0	(0)
#define	R_D1	(1)
#define	R_D2	(2)
#define	R_D3	(3)
#define	R_D4	(4)
#define	R_D5	(5)
#define	R_D6	(6)
#define	R_D7	(7)
#define R_A0	(8)
#define	R_A1	(9)
#define	R_A2	(10)
#define	R_A3	(11)
#define	R_A4	(12)
#define	R_A5	(13)
#define	R_A6	(14)


/* kercall argument aliases	*/
#define	KERCALL		R_D0
#define TARGET		R_D1
	/* message part		*/
#define M_OPCODE	R_D2
#define M_HANDLE	R_D3
#define M_PARAM0	R_D4
#define M_PARAM1	R_D5
#define M_PARAM2	R_D6
#define M_PARAM3	R_D7
#define M_DWADDR	R_A0
#define M_DWSIZE	R_A1
#define	M_INDEX		R_A2


/* reply argument aliases	*/
#define	R_STATUS	R_D0
	/* reply message part	*/
#define R_OPCODE	R_D2
#define R_HANDLE	R_D3
#define R_PARAM0	R_D4
#define R_PARAM1	R_D5
#define R_PARAM2	R_D6
#define R_PARAM3	R_D7


/* aliases associated with specific kercalls */

/* argument format for sysport() call	*/
#define	PORT_ENTRY	M_PARAM0
#define PORT_PASSPORT	M_PARAM1
#define	PORT_PRIORITY	M_PARAM2

/* aliases relevant to sysjump() call	*/
#define JUMP_PASSPORT	R_D1
#define JUMP_DWADDR	R_A0
#define JUMP_DWSIZE	R_A1
#define	JUMP_ENTRY	R_A2

/* aliases relevant to sysspl() call	*/
#define THREAD_PRIORITY	M_PARAM0
