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


|/*
| *	TRIX 1.0
| *	Assembly Language Interface
| *
| *	Specific Kernel Calls:
| *		t_REQUEST(target, mp)
| *		t_REPLY(mp)
| *		t_RELAY(target, mp)
| *
| *	General Kernel Call:
| *		t_CALL(args)
| *
| *	Other Traps:
| *		traceon()
| *		traceoff()
| *		resched()
| *		debugon()
| *		debugoff()
| *		profon()
| *		profoff()
| *
| *	This code executes on the user stack:
| *		(1) save user registers on stack
| *		(2) pass kernel-call args in registers
| *		(3) trap to kernel
| *		(4) record return values in args
| *		(5) restore user registers from stack
| */


K_REQUEST  = 1
K_REPLY	   = 2
K_RELAY	   = 3

	.text

|/*
| * t_REQUEST(target, mp)
| *	hndl_t	target;
| *	struct	message *mp;
| *
| * returns reply-opcode
| */
	.globl	_t_REQUEST
_t_REQUEST:
	link	a6,#0
	moveml	#0x3F3F,sp@-	| save d2-d7, a2-a7
	movl	#K_REQUEST,d0	| KERCALL
	movl	a6@(8),d1	| TARGET
	movl	a6@(12),a5	| a5 points to (request/reply)-message
	moveml	a5@,#0xFFC	| load d2-d7, a0-a3 with request-message
	movl	a5,sp@-		| record reply-message pointer
|*********************************
	trap	#0		| trap to kernel
|*********************************
	movl	sp@+,a5		| a5 points to reply-message
	movl	d2,d0		| return reply-opcode in d0
	moveml	#0xFFC,a5@	| store reply-message
	moveml	sp@+,#0x7CFC	| restore d2-d7, a2-a6
	addql	#4,sp		| fix stack pointer
	unlk	a6
	rts
	

|/*
| * t_REPLY(mp)
| *	struct	message *mp;
| *
| * no return
| */
	.globl	_t_REPLY
_t_REPLY:
	link	a6,#0
	moveml	#0x3F3F,sp@-	| save d2-d7, a2-a7
	movl	#K_REPLY,d0	| KERCALL
	movl	a6@(8),a5	| a5 points to reply-message
	moveml	a5@,#0xFFC	| load d2-d7, a0-a3 with reply-message
|*********************************
	trap	#0		| trap to kernel
|*********************************
	bra	.		| no return


|/*
| * t_RELAY(target, mp)
| *	hndl_t	target;
| *	struct	message *mp;
| *
| * if (valid target) no return, else returns error-code
| */
	.globl	_t_RELAY
_t_RELAY:
	link	a6,#0
	moveml	#0x3F3F,sp@-	| save d2-d7, a2-a7
	movl	#K_RELAY,d0	| KERCALL
	movl	a6@(8),d1	| TARGET
	movl	a6@(12),a5	| a5 points to jump-message
	moveml	a5@,#0xFFC	| load d2-d7, a0-a3 with relayed message
|*********************************
	trap	#0		| trap to kernel
|*********************************
	movl	d2,d0		| return any error-code in d0
	moveml	sp@+,#0x7CFC	| restore d2-d7, a2-a6
	addql	#4,sp		| fix stack pointer
	unlk	a6
	rts


|/*
| * t_CALL(args)
| *	arg_t	args[N_ARGS]
| */
	.globl	_t_CALL
_t_CALL:
	link	a6,#0
	moveml	#0x3F3F,sp@-	| save d2-d7, a2-a7
	movl	a6@(8),a5	| a5 points to call args
	moveml	a5@,#0xFFF	| load d0-d7, a0-a3 with args
	movl	a5,sp@-		| record arg pointer
|*********************************
	trap	#0		| trap to kernel
|*********************************
	movl	sp@+,a5		| a5 points to call args
	moveml	#0xFFF,a5@	| store return args (d0 = status)
	moveml	sp@+,#0x7CFC	| restore d2-d7, a2-a6
	addql	#4,sp		| fix stack pointer
	unlk	a6
	rts


	.globl	_traceon
_traceon:
	link	a6,#0
	moveml	#0x3F3F,a7@-	| save d2-d7, a2-a7
|*********************************
	trap	#1		| trap to kernel
|*********************************
	moveml	a7@+,#0x7CFC	| restore d2-d7, a2-a6
	addql	#4,a7
	unlk	a6
	rts


	.globl	_traceoff
_traceoff:
	link	a6,#0
	moveml	#0x3F3F,a7@-	| save d2-d7, a2-a7
|*********************************
	trap	#2		| trap to kernel
|*********************************
	moveml	a7@+,#0x7CFC	| restore d2-d7, a2-a6
	addql	#4,a7
	unlk	a6
	rts


	.globl	_resched
_resched:
	link	a6,#0
	moveml	#0x3F3F,a7@-	| save d2-d7, a2-a7
|*********************************
	trap	#3		| trap to kernel
|*********************************
	moveml	a7@+,#0x7CFC	| restore d2-d7, a2-a6
	addql	#4,a7
	unlk	a6
	rts


	.globl	_debugon
_debugon:
	link	a6,#0
	moveml	#0x3F3F,a7@-	| save d2-d7, a2-a7
|*********************************
	trap	#4		| trap to kernel
|*********************************
	moveml	a7@+,#0x7CFC	| restore d2-d7, a2-a6
	addql	#4,a7
	unlk	a6
	rts


	.globl	_debugoff
_debugoff:
	link	a6,#0
	moveml	#0x3F3F,a7@-	| save d2-d7, a2-a7
|*********************************
	trap	#5		| trap to kernel
|*********************************
	moveml	a7@+,#0x7CFC	| restore d2-d7, a2-a6
	addql	#4,a7
	unlk	a6
	rts


	.globl	_profon
_profon:
	link	a6,#0
	moveml	#0x3F3F,a7@-	| save d2-d7, a2-a7
|*********************************
	trap	#6		| trap to kernel
|*********************************
	moveml	a7@+,#0x7CFC	| restore d2-d7, a2-a6
	addql	#4,a7
	unlk	a6
	rts


	.globl	_profoff
_profoff:
	link	a6,#0
	moveml	#0x3F3F,a7@-	| save d2-d7, a2-a7
|*********************************
	trap	#7		| trap to kernel
|*********************************
	moveml	a7@+,#0x7CFC	| restore d2-d7, a2-a6
	addql	#4,a7
	unlk	a6
	rts
