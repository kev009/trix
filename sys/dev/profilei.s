|/*
| * TRIX 1.0
| * Profile Timer Interrupt Catcher
| *
| *	struct	kprof
| *	{
| *		long	kp_scale;
| *		long	kp_base;
| *		long	kp_size;
| *		char	*kp_ptr;
| *	};
| */

	.text
	.globl	_ikprof, tkprof, totcnt, usrcnt, syscnt, kercnt, insched, scnt
	.globl	kcall, kcnt
	.globl	D_CURREN, D_SYSTEM

_ikprof:
	moveml	#0xC080,sp@-	| save d0-d1, a0 on stack
	addql	#1,totcnt	| increment total-counter
	movw	sp@(12),d0	| d0 = interrupt ps
	andw	#0x2000,d0	| in kernel mode?
	beq	1$		| 	no - check domain
	addql	#1,kercnt	| 	yes - increment kernel-counter
	bra	3$		| do profile
1$:	movl	D_SYSTEM,d0	| d0 = kernel domain pointer
	cmpl	D_CURREN,d0	| are we in the kernel domain?
	beq	2$		| 	yes - increment system-counter
	addql	#1,usrcnt	| 	no - increment user-counter
	bra	4$		| return
2$:	addql	#1,syscnt	| increment system-counter
3$:	movl	insched,d0
	beq	6$
	addql	#1,scnt

6$:	movl	kcall,d0
	beq	5$
	addql	#1,kcnt

5$:	movl	sp@(14),d0	| d0 = interrupt pc
	lea	tkprof,a0	| a0 = (struct kprof *)tkprof
	subl	a0@(4),d0	| offset = pc - tkprof.kp_base
	blt	4$		| if (offset < 0) return
	movl	a0@,d1		| shift count in d1
	asrl	d1,d0		| offset >> tkprof.kp_scale
	andl	#0xFFFE,d0	| d0 = word aligned index
	cmpl	a0@(8),d0	| does it fit in profile-buffer?
	bge	4$		| 	no - return
	movl	a0@(12),a0	| a0 = tkprof.kp_ptr
	addl	d0,a0		| a0 = index to word in profile-buffer
	addqw	#1,a0@		| increment profile-buffer word-counter
4$:	moveml	sp@+,#0x103	| restore d0-d1, a0 from stack
	rte

