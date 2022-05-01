|***********************************************************************|
|	Startup and Porch code for TRIX 1.0				|
|***********************************************************************|

PRI0 = 0x000			| low priority (spl 0)
PRI7 = 0x700			| high priority (spl 7)
PRIMSK = 0x700			| priority bits in ps
KERNEL = 0x2000			| kernel mode bit in ps

PAGESIZE= 0x400			| size of one page
ARGVSIZE= 0x100			| size of arguments passed in from loader
INITIALZ= 255			| trap number to call main
SCHEDULE= 256			| trap number to reschedule

	.text
	.globl	__text, _edata, _end
	.globl	main, panic

__text:
start:	tstl	d0		| if a0 != NULL copy string to argv
	jeq	3$
	movl	d0,a0
	movl	#argv,a1
	movl	#ARGVSIZE-1,d0
4$:	movb	a0@+,a1@+	| copy the string
	subl	#1,d0
	jgt	4$

3$:	movl	#_end,d0	| end of trix
	addl	#PAGESIZE-1,d0	| round up to next 1k
	andl	#~0x3FF,d0
	movl	d0,usrcore	| address of first page following kernel
	movl	#_edata,a0	| start clearing here
1$:	clrl	a0@+		| clear bss
	cmpl	a0,d0
	jne	1$

	movl	#0xE0000,a0	| start of memory
2$:	addl	#0x20000,a0	| bump to next board of memory
	movw	#0x1234,a0@	| try to write into memory
	cmpw	#0x1234,a0@	| did we succeed?
	jeq	2$		| yes, keep going
	subl	#PAGESIZE,a0	| avoid top page
	movl	a0,maxcore	| else this must be top of memory

	movl	#0x100000,sp	| kernel stack grows down from here
	movw	#PRI7+KERNEL,sr	| go to high priority
	link	a6,#0		|  initialize frame pointer
	link	a6,#0
	link	a6,#0
	clrl	sp@-		| fake pushed pc
	clrw	sp@-		| fake pushed ps (user mode)
	jsr	fault		| call into trap to start up system
.word	INITIALZ
	movl	#9$,sp@-	| call panic if system thread returns
	jsr	panic
	bra	.

.data
9$:
.asciz	"system thread returned"
.even
.text


| primitive form of ignore interrupt:
	.globl	 rte
rte:	rte


| prototype dispatch table entry, this is copied to each word in dispatch:
|
| trap and fault entry, called like:
|
|	jsr	fault		|leaves ptr to following word on stack
|	.word	n		|n will be fault number

	.data
	.globl proto, prindex
proto:	jsr	fault
prindex:.word	0


	.text

	.globl	fault, trap
	.globl	dosched
	.globl	T_CURREN

|  fault is called from an interrupt or exception:
|    jsr  fault ; .word  faultnumber
|
|  fault saves the registers in the thread if the call is from umode
|    and calls the c routine trap()
|  the form of the call is:
|    trap(number, usp, d0, d1, a0, a1, padding, ups, upc)
|
|  after trap returns the registers are restored from the thread if the return
|    is to umode

fault:	clrw	sp@-		| this makes sp long aligned (psw is a short)
	moveml	#0xC0C0,sp@-	| push d0-d1 a0-a1
	movl	usp,a0
	movl	a0,sp@-		| push usp

	movl	sp@(24),d0	| ps
	andl	#KERNEL,d0	| are we coming from kernel mode?
	bne	nosav		| don't save registers if from kernel mode

	movl	T_CURREN,a0
	moveml	#0x7FFF,a0@	| save registers d0-d7 a0-a6 in thread struct
	movl	sp@(4),a0@(0)	| save real d0 in thread struct
	movl	sp@(12),a0@(32)	| save real a0 in thread struct

nosav:	movl	sp@(22),a0	| return pc from  jsr fault
	movw	a0@,sp@-	| push fault number
	clrw	sp@-		|   extended to a long

dotrap:	jsr	trap		| C handler for traps and faults

	addql	#4,sp		| pop fault number

	movl	sp@+,a0
	movl	a0,usp		| restore usp
	movl	sp@(20),d0	| get original ps
	andl	#KERNEL,d0	| did we come from user mode?
	moveml	sp@+,#0x0303	| first restore d0-d1 a0-a1 (leave ccodes)
	jne	norst		|   no, just continue
	tstl	dosched		| should we reschedule?
	jeq	1$		|   no, just return normally
	moveml	#0xC0C0,sp@-	| save d0-d1 a0-a1 again
	movl	usp,a0
	movl	a0,sp@-		| save usp again
	movl	#SCHEDULE,sp@-	| reschedule trap number
	jra	dotrap		| go back into trap

1$:	movl	T_CURREN,a0
	moveml	a0@,#0x7FFF	| restore registers d0-d7 a0-a6 from thread

norst:	addql	#6,sp		| pop fault pc, and alignment word
	rte


| Bus error entry, this has its stack somewhat different.  We will
| call a C routine to save the info then fix the stack to look like a trap.
| These entries will be called directly from interrupt vector.

	.globl	buserr,busaddr
buserr:	moveml	#0xC0C0,sp@-	| save registers that C clobbers
	jsr	busaddr		| save the info for a bus or address error
	moveml	sp@+,#0x303	| restore registers
	addql	#8,sp		| pop fcode, aaddr and ireg
	jsr	fault
	.word	2

	.globl	addrerr
addrerr:moveml	#0xC0C0,sp@-	| save registers that C clobbers
	jsr	busaddr		| save the info for a bus or address error
	moveml	sp@+,#0x303	| restore registers
	addql	#8,sp		| pop fcode, aaddr and ireg
	jsr	fault
	.word	3


|  Set-Priority-Level Trap

	.globl	spltrap, D_CURREN, D_SYSTEM
spltrap:
	movw	sp@,d0		| d0 = interrupt ps
	andl	#KERNEL,d0	| in kernel mode?
	bne	1$		|	yes - set priority level
	movl	D_SYSTEM,d0	| d0 = kernel domain pointer
	cmpl	D_CURREN,d0	| are we in the kernel domain?
	beq	1$		|	yes - set priority level
	moveq	#-1,d0		| d0 = -1
	rte
1$:
	andl	#PRIMSK,d1	| only set priority bits
	movw	sp@,d0
	andl	#KERNEL,d0	| yup, get the KERNEL bit again
	orl	d0,d1		| use old KERNEL bit
	movw	sp@,d0		| return old ps value
	andl	#PRIMSK,d0	| only return priority bits
	movw	d1,sp@
	rte


| hpl - sets hardware priority level

	.globl	hpl7, hpl0, hplx

hpl7:	movw	sr,d0
	movw	#PRI7+KERNEL,sr
	rts

hpl0:	movw	sr,d0
	movw	#PRI0+KERNEL,sr
	rts

hplx:	movw	sp@(6),sr
	rts

	.data
	.globl	lowcore, usrcore, maxcore
lowcore:.long	0
usrcore:.long	0
maxcore:.long	0

	.globl	argv
argv:	. = . + 512
