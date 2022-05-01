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
| * Memory Assist Routines
| */

|/*
| * mfill(source, count, data)
| * char *source;
| * int	 count;
| * char data;
| *
| * write 'count' bytes of 'data' starting at 'source'
| */
	.globl _mfill
	.text

_mfill:	movl	sp@(8),d0	|count
	jeq	8$		|nothing to do

	movb	sp@(15),d1	|data
	lea	filld,a1	|setup data
	movb	d1,a1@+		|first long
	movb	d1,a1@+
	movb	d1,a1@+
	movb	d1,a1@+
	movl	filld,a0
	moveq	#12,d1		|loop count
1$:	movl	a0,a1@+		|record rest of data
	subql	#1,d1		|decrement count
	jgt	1$		|done?

	movl	sp@(4),d1	|source
	addl	d0,d1		|&p[n]
	movl	d1,a0		|save it
	andl	#1,d1		|word aligned?
	jeq	2$		|yes, potentially long moves
	movb	filld,a0@-	|fill up to word boundry
	subql	#1,d0		|one less byte to fill
	jeq	8$		|nothing left

2$:	movl	d0,d1		|copy n
	andl	#~0xFF,d1	|m = number of 256 byte blocks left * 256
	jeq	4$		|none

	subl	d1,d0		|we will do this many bytes in next loop
	asrl	#8,d1		|number of blocks left
	moveml	#0xFF7E,sp@-	|save registers
	movl	d1,sp@-		|number of blocks goes on top of stack
	lea	filld,a1
	moveml	a1@,#0x7CFF	|fill out a bunch of registers
	movl	d0,a1		|and this one too

3$:	moveml	#0xFF7E,a0@-	|fill out 14 longs worth
	moveml	#0xFF7E,a0@-	|fill out 14 longs worth
	moveml	#0xFF7E,a0@-	|fill out 14 longs worth
	moveml	#0xFF7E,a0@-	|fill out 14 longs worth
	moveml	#0xFF00,a0@-	|fill out 8 longs worth, total of 256 bytes
	subql	#1,sp@		|one more block, any left?
	jgt	3$		|yes, do another pass

	movl	sp@+,d1		|just pop stack
	moveml	sp@+,#0x7EFF	|give me back the registers

4$:	movl	d0,d1		|copy n left
	andl	#~3,d1		|this many longs left
	jeq	6$		|none
	subl	d1,d0		|do this many in next loop

5$:	movl	filld,a0@-	|fill a long's worth
	subql	#4,d1		|this many bytes in a long
	jgt	5$		|if there are more

6$:	tstl	d0		|anything left?
	jeq	8$		|no, just stop here

7$:	movb	filld,a0@-	|fill 1 byte's worth
	subql	#1,d0		|one less byte to do
	jgt	7$		|if any more

8$:	rts			|that's it

	.data
filld:	.long	0,0,0,0,0,0,0,0,0,0,0,0,0	|room for 13 longs of data


|/*
| * mcopy(source, destination, count)
| * char *source, *destination;
| * int	 count;
| *
| * copies 'count' bytes beginning at 'source' to 'destination'
| */
	.globl	_mcopy
	.text

_mcopy:	movl	sp@(4),a1	|source
	movl	sp@(8),a0	|destination
	movl	a0,d0		|destination
	andl	#0x1,d0		|see if word aligned
	movl	a1,d1		|source
	andl	#0x1,d1		|see if long word aligned?
	cmpl	d0,d1		|do they agree?
	beq	1$		|yes, can do something interesting
	movl	sp@(12),d1	|count
	bra	5$		|ho, hum, just do byte moves
1$:	movl	sp@(12),d1	|count
	tstl	d0		|are we on a long boundry?
	beq	3$		|yes, don't worry about fudge
	negl	d0		|complement
	addql	#2,d0		|2 - adjustment = fudge
	cmpl	d1,d0		|is count bigger than fudge
	bge	5$		|no, must be 3 bytes or less
	subl	d0,d1		|shrink remaining count by this much
	subql	#1,d0		|dbf is a crock
2$:	movb	a1@+,a0@+	|move bytes to get to long boundry
	subql	#1,d0		|while alignment count
	bge	2$
3$:	movl	d1,d0		|copy remaining count
	andl	#~3,d0		|count mod 4 is number of long words
	beq	5$		|hmm, must not be any
	subl	d0,d1		|long words moved * 4 = bytes moved
	asrl	#2,d0		|number of long words
	cmpl	#12,d0		|do we have a bunch to do?
	ble	38$		|no, just do normal moves
	moveml	#0x7F3E,sp@-	|save some registers
34$:	moveml	a1@+,#0x7CFE	|block move via various registers
	moveml	#0x7CFE,a0@
	addl	#48,a0		|moveml won't let me auto inc a destination
	subl	#12,d0		|we moved twelve longs worth
	cmpl	#12,d0		|do we have another 12 to go
	bgt	34$		|yes, keep at it
	moveml	sp@+,#0x7CFE	|restore registers
	tstl	d0		|any long's left
	beq	5$		|no, nothing but a few random bytes
38$:	subql	#1,d0		|dbf is a crock
4$:	movl	a1@+,a0@+	|copy as many long words as possible
	subql	#1,d0		|while long word count
	bge	4$
5$:	tstl	d1		|anything left to do?
	beq	7$		|nothing left
	subql	#1,d1		|dbf is a crock
6$:	movb	a1@+,a0@+	|copy any residual bytes
	subql	#1,d1		|while byte count
	bge	6$
7$:	movl	sp@(12),d0	|just return the count
	rts
