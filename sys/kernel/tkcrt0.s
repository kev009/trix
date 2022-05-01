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


|/***********************************************************************
|
|	TRIX 1.0
|	Standard Kernel Domain Request Entry Interface
|
|	This code executes on the user stack and is called when a
|	request goes to a kernel object/handler.  At domain entry
|	registers contain:
|
|		d1	->	passport
|		d2-d7	->	message opcode, handle, parameters
|		a0	->	message data-window-base
|		a1	->	message data-window-size
|		a2	->	handler address
|		a3	->	uid
|
|	The user stack is fixed so the target handler routine gets
|	called as:
|
|		handler(passport, (struct message *)mp)
|
|	Upon return from the handler, the reply message is expected
|	to reside in the request message location.
|
| ***********************************************************************/

K_REPLY = 2			| kernel call to reply from request

	.text
	.globl	_krequest

_krequest:
	moveml	#0x3FF0,sp@-	| push message on stack
	movl	sp,d0		| d0 contains message address
	movl	d0,sp@-		| push message address on stack
	movl	d1,sp@-		| push passport on stack
	jsr	a2@		| call the handler

	addl	#8,sp		| pop handler args from stack
	moveml	sp@+,#0xFFC	| pop reply message from stack
	movl	#K_REPLY,d0	| KERCALL set to reply

	trap	#0		| trap to kernel

	movl	#krmsg,sp@-	| call panic if reply trap returns
	jsr	_panic
	bra	.

	.data
krmsg:	.asciz	"krequest reply trap returned"
	.even
