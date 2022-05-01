ARGUMENT = 0
|/*
| *	Prototype Device Interrupt Catcher
| *	Prototype Ignore Interrupt Catcher
| */
	.text
	.globl	_iproto, _ignore
_iproto:
	moveml	#0xC0C0,sp@-	| save registers d0-d1, a0-a1
	movl	#ARGUMENT,sp@-	| argument to handler
	jsr	HANDLER		| call interrupt handler
	addql	#4,sp		| pop handler argument from stack
	moveml	sp@+,#0x303	| restore registers d0-d1, a0-a1
	rte			| return from interrupt
_ignore:
	rte
HANDLER:
	rts
