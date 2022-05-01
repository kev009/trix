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
| * Assembly Language Utility Routines
| */

|addressed unsigned long division: *dividend = *dividend / divisor

	.globl	auldiv
	.text

auldiv:	link	a6,#0
	moveml	#0x3800,sp@-	|need d2,d3,d4 registers
	movl	a6@(8),a0	|a0 = dividend pointer
	movl	a0@,d0		|d0 = dividend
	movl	d0,d3		|save dividend
	movl	a6@(12),d1	|divisor
	movl	d1,d4		|save divisor

	cmpl	#0x10000,d1	|divisor >= 2 ** 16?
	jge	1$		|yes, divisor must be < 2 ** 16
	clrw	d0		|divide dividend
	swap	d0		|  by 2 ** 16
	divu	d1,d0		|get the high order bits of quotient
	movw	d0,d2		|save quotient high
	movw	d3,d0		|dividend low + remainder * (2**16)
	divu	d1,d0		|get quotient low
	swap	d0		|temporarily save quotient low in high
	movw	d2,d0		|restore quotient high to low part of register
	swap	d0		|put things right
	jra	3$		|return

1$:	asrl	#0x1,d0		|shift dividend
	asrl	#0x1,d1		|shift divisor
	andl	#0x7FFFFFFF,d0	|assure positive
	andl	#0x7FFFFFFF,d1	|  sign bit
	cmpl	#0x10000,d1	|divisor < 2 ** 16?
	jge	1$		|no, continue shift
	divu	d1,d0		|yes, divide, remainder is garbage
	andl	#0xFFFF,d0	|get rid of remainder
	movl	d0,d2		|save quotient
	movl	d0,sp@-		|call ulmul with quotient
	movl	d4,sp@-		|  and saved divisor on stack
	jsr	ulmul		|  as arguments
	addql	#0x8,sp		|restore sp
	cmpl	d0,d3		|original dividend >= lmul result?
	jge	2$		|yes, quotient should be correct
	subql	#1,d2		|no, fix up quotient
2$:	movl	d2,d0		|move quotient to d0

3$:	movl	d0,a0@		|store result via pointer
	moveml	sp@+,#0x1C	|restore registers
	unlk	a6
	rts


|addressed signed long division: *dividend = *dividend/divisor

	.globl	aldiv
	.text

aldiv:	link	a6,#0
	moveml	#0x3C00,sp@-	|need d2,d3,d4,d5 registers
	movl	#1,d5		|sign of result
	movl	a6@(8),a0	|a0 = dividend pointer
	movl	a0@,d0		|d0 = dividend
	jge	1$
	negl	d0
	negl	d5
1$:	movl	d0,d3		|save positive dividend
	movl	a6@(12),d1	|divisor
	jge	2$
	negl	d1
	negl	d5
2$:	movl	d1,d4		|save positive divisor

	cmpl	#0x10000,d1	|divisor < 2 ** 16?
	jge	3$		|no, divisor must be < 2 ** 16
	clrw	d0		|yes, divide dividend
	swap	d0		|  by 2 ** 16
	divu	d1,d0		|get the high order bits of quotient
	movw	d0,d2		|save quotient high
	movw	d3,d0		|dividend low + remainder*(2**16)
	divu	d1,d0		|get quotient low
	swap	d0		|temporarily save quotient low in high
	movw	d2,d0		|restore quotient high to low part of register
	swap	d0		|put things right
	jra	5$		|return

3$:	asrl	#0x1,d0		|shift dividend
	asrl	#0x1,d1		|shift divisor
	andl	#0x7FFFFFFF,d0	|assure positive
	andl	#0x7FFFFFFF,d1	|  sign bit
	cmpl	#0x10000,d1	|divisor < 2 ** 16?
	jge	3$		|no, continue shift
	divu	d1,d0		|yes, divide, remainder is garbage
	andl	#0xFFFF,d0	|get rid of remainder
	movl	d0,d2		|save quotient
	movl	d0,sp@-		|call ulmul with quotient
	movl	d4,sp@-		|  and saved divisor on stack
	jsr	ulmul		|  as arguments
	addql	#0x8,sp		|restore sp
	cmpl	d0,d3		|original dividend >= lmul result?
	jge	4$		|yes, quotient should be correct
	subql	#1,d2		|no, fix up quotient
4$:	movl	d2,d0		|move quotient to d0

5$:	tstl	d5		|sign of result
	jge	6$
	negl	d0
6$:	movl	d0,a0@		|store result via pointer
	moveml	sp@+,#0x3C	|restore registers
	unlk	a6
	rts


|signed long division: quotient = dividend / divisor

	.globl	ldiv
	.text

ldiv:	link	a6,#0
	moveml	#0x3C00,sp@-	|need d2,d3,d4,d5 registers
	movl	#1,d5		|sign of result
	movl	a6@(8),d0	|dividend
	jge	1$
	negl	d0
	negl	d5
1$:	movl	d0,d3		|save positive dividend
	movl	a6@(12),d1	|divisor
	jge	2$
	negl	d1
	negl	d5
2$:	movl	d1,d4		|save positive divisor

	cmpl	#0x10000,d1	|divisor >= 2 ** 16?
	jge	3$		|yes, divisor must be < 2 ** 16
	clrw	d0		|divide dividend
	swap	d0		|  by 2 ** 16
	divu	d1,d0		|get the high order bits of quotient
	movw	d0,d2		|save quotient high
	movw	d3,d0		|dividend low + remainder * (2**16)
	divu	d1,d0		|get quotient low
	swap	d0		|temporarily save quotient low in high
	movw	d2,d0		|restore quotient high to low part of register
	swap	d0		|put things right
	jra	5$		|return

3$:	asrl	#0x1,d0		|shift dividend
	asrl	#0x1,d1		|shift divisor
	andl	#0x7FFFFFFF,d0	|insure positive
	andl	#0x7FFFFFFF,d0	|  sign bit
	cmpl	#0x10000,d1	|divisor < 2 ** 16?
	jge	3$		|no, continue shift
	divu	d1,d0		|yes, divide, remainder is garbage
	andl	#0xFFFF,d0	|get rid of remainder
	movl	d0,d2		|save quotient
	movl	d0,sp@-		|call ulmul with quotient
	movl	d4,sp@-		|  and saved divisor on stack
	jsr	ulmul		|  as arguments
	addql	#0x8,sp		|restore sp
	cmpl	d0,d3		|original dividend >= lmul result?
	jge	4$		|yes, quotient should be correct
	subql	#1,d2		|no, fix up quotient

4$:	movl	d2,d0		|move quotient to d0
5$:	tstl	d5		|sign of result
	jge	6$
	negl	d0
6$:	moveml	sp@+,#0x3C	|restore registers
	unlk	a6
	rts


|signed long remainder: a = a % b

	.globl	lrem
	.text

lrem:	link	a6,#0
	moveml	#0x3800,sp@-	|need d2,d3,d4 registers
	movl	#1,d4		|sign of result
	movl	a6@(8),d0	|dividend
	bge	1$
	negl	d0
	negl	d4
1$:	movl	d0,d2		|save positive dividend
	movl	a6@(12),d1	|divisor
	bge	2$
	negl	d1

2$:	cmpl	#0x10000,d1	|divisor < 2 ** 16?
	bge	3$		|no, divisor must be < 2 ** 16
	clrw	d0		|d0 =
	swap	d0		|   dividend high
	divu	d1,d0		|yes, divide
	movw	d2,d0		|d0 = remainder high + quotient low
	divu	d1,d0		|divide
	clrw	d0		|d0 = 
	swap	d0		|   remainder
	bra	6$		|return

3$:	movl	d1,d3		|save divisor
4$:	asrl	#0x1,d0		|shift dividend
	asrl	#0x1,d1		|shift divisor
	andl	#0x7FFFFFFF,d0	|assure positive
	andl	#0x7FFFFFFF,d1	|  sign bit
	cmpl	#0x10000,d1	|divisor < 2 ** 16?
	bge	4$		|no, continue shift
	divu	d1,d0		|yes, divide
	andl	#0xFFFF,d0	|erase remainder
	movl	d0,sp@-		|call ulmul with quotient
	movl	d3,sp@-		|  and saved divisor on stack
	jsr	ulmul		|  as arguments
	addql	#0x8,sp		|restore sp
	cmpl	d0,d2		|original dividend >= lmul result?
	jge	5$		|yes, quotient should be correct
	subl	d3,d0		|no, fixup 
5$:	subl	d2,d0		|calculate
	negl	d0		|  remainder

6$:	tstl	d4		|sign of result
	bge	7$
	negl	d0
7$:	moveml	sp@+,#0x1C	|restore registers
	unlk	a6
	rts


|signed long multiply: c = a * b

	.globl	lmul
	.text

lmul:	link	a6,#0
	moveml	#0x3800,sp@-	|save d2,d3,d4
	movl	#1,d4		|sign of result
	movl	a6@(8),d2	|d2 = a
	bge	1$
	negl	d2
	negl	d4
1$:	movl	a6@(12),d3	|d3 = b
	bge	2$
	negl	d3
	negl	d4

2$:	clrl	d0
	movw	d2,d0		|d0 = alo, unsigned
	mulu	d3,d0		|d0 = blo*alo, unsigned
	movw	d2,d1		|d1 = alo
	swap	d2		|swap alo-ahi
	mulu	d3,d2		|d2 = blo*ahi, unsigned
	swap	d3		|swap blo-bhi
	mulu	d3,d1		|d1 = bhi*alo, unsigned
	addl	d2,d1		|d1 = (ahi*blo + alo*bhi)
	swap	d1		|d1 =
	clrw	d1		|   (ahi*blo + alo*bhi)*(2**16)
	addl	d1,d0		|d0 = alo*blo + (ahi*blo + alo*bhi)*(2**16)
	tstl	d4		|sign of result
	bge	3$
	negl	d0

3$:	moveml	sp@+,#0x1C	|restore d2,d3,d4
	unlk	a6
	rts


|unsigned long division: dividend = dividend / divisor

	.globl	uldiv
	.text

uldiv:	link	a6,#0
	moveml	#0x3800,sp@-	|need d2,d3,d4 registers
	movl	a6@(8),d0	|dividend
	movl	d0,d3		|save dividend
	movl	a6@(12),d1	|divisor
	movl	d1,d4		|save divisor

	cmpl	#0x10000,d1	|divisor >= 2 ** 16?
	jge	1$		|yes, divisor must be < 2 ** 16
	swap	d0		|divide dividend
	andl	#0xFFFF,d0	|  by 2 ** 16
	divu	d1,d0		|get the high order bits of quotient
	movw	d0,d2		|save quotient high
	movw	d3,d0		|dividend low + remainder * (2**16)
	divu	d1,d0		|get quotient low
	swap	d0		|temporarily save quotient low in high
	movw	d2,d0		|restore quotient high to low part of register
	swap	d0		|put things right
	jra	3$		|return

1$:	asrl	#0x1,d0		|shift dividend
	asrl	#0x1,d1		|shift divisor
	andl	#0x7FFFFFFF,d0	|assure positive
	andl	#0x7FFFFFFF,d1	|  sign bit
	cmpl	#0x10000,d1	|divisor < 2 ** 16?
	jge	1$		|no, continue shift
	divu	d1,d0		|yes, divide, remainder is garbage
	andl	#0xFFFF,d0	|get rid of remainder
	movl	d0,d2		|save quotient
	movl	d0,sp@-		|call ulmul with quotient
	movl	d4,sp@-		|  and saved divisor on stack
	jsr	ulmul		|  as arguments
	addql	#0x8,sp		|restore sp
	cmpl	d0,d3		|original dividend >= lmul result?
	jge	2$		|yes, quotient should be correct
	subql	#1,d2		|no, fix up quotient
2$:	movl	d2,d0		|move quotient to d0

3$:	moveml	sp@+,#0x1C	|restore registers
	unlk	a6
	rts


|unsigned long remainder: a = a % b

	.globl	ulrem
	.text

ulrem:	link	a6,#0
	moveml	#0x3000,sp@-	|need d2,d3 registers
	movl	a6@(8),d0	|dividend
	movl	d0,d2		|save dividend
	movl	a6@(12),d1	|divisor

	cmpl	#0x10000,d1	|divisor < 2 ** 16?
	bge	1$		|no, divisor must be < 2 ** 16
	clrw	d0		|d0 =
	swap	d0		|   dividend high
	divu	d1,d0		|yes, divide
	movw	d2,d0		|d0 = remainder high + quotient low
	divu	d1,d0		|divide
	clrw	d0		|d0 = 
	swap	d0		|   remainder
	bra	4$		|return

1$:	movl	d1,d3		|save divisor
2$:	asrl	#0x1,d0		|shift dividend
	asrl	#0x1,d1		|shift divisor
	andl	#0x7FFFFFFF,d0	|assure positive
	andl	#0x7FFFFFFF,d1	|  sign bit
	cmpl	#0x10000,d1	|divisor < 2 ** 16?
	bge	2$		|no, continue shift
	divu	d1,d0		|yes, divide
	andl	#0xFFFF,d0	|erase remainder
	movl	d0,sp@-		|call ulmul with quotient
	movl	d3,sp@-		|  and saved divisor on stack
	jsr	ulmul		|  as arguments
	addql	#0x8,sp		|restore sp
	cmpl	d0,d2		|original dividend >= lmul result?
	jge	3$		|yes, quotient should be correct
	subl	d3,d0		|no, fixup 
3$:	subl	d2,d0		|calculate
	negl	d0		|  remainder

4$:	moveml	sp@+,#0xC	|restore registers
	unlk	a6
	rts


|unsigned long multiply: c = a * b

	.globl	ulmul
	.text

ulmul:	link	a6,#0
	moveml	#0x3000,sp@-	|save d2,d3
	movl	a6@(8),d2	|d2 = a
	movl	a6@(12),d3	|d3 = b

	clrl	d0
	movw	d2,d0		|d0 = alo, unsigned
	mulu	d3,d0		|d0 = blo*alo, unsigned
	movw	d2,d1		|d1 = alo
	swap	d2		|swap alo-ahi
	mulu	d3,d2		|d2 = blo*ahi, unsigned
	swap	d3		|swap blo-bhi
	mulu	d3,d1		|d1 = bhi*alo, unsigned
	addl	d2,d1		|d1 = (ahi*blo + alo*bhi)
	swap	d1		|d1 =
	clrw	d1		|   (ahi*blo + alo*bhi)*(2**16)
	addl	d1,d0		|d0 = alo*blo + (ahi*blo + alo*bhi)*(2**16)

3$:	moveml	sp@+,#0xC	|restore d2,d3
	unlk	a6
	rts

