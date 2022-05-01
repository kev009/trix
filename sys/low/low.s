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


| memory managment constants
SYSTEM	= 0xF00000	| address of system/kernel space
PGSHIFT	= 10		| log2 of pagesize
L2MASK	= 2048-4	| (512-1)<<2
MAXCID	= 63		| number of usable cache ids
NOTLB	= 0xC3		| turn off translation
DOTLB	= 0x3C		| turn on translation

| psw masks
HIGH	= 0x2700	| high priority kernel mode
PRIMSK	= 0x0700	| mask to get priority bits
PRI0	= 0x0000	| priority level 0 (high)
PRI7	= 0x0700	| priority level 7 (low)
SMODE	= 0x2000	| kernel mode bit
SBITNUM	= 5		| kernel mode bit in psw

| masks for saving and restoring registers
CSAVE	= 0xC0C0	| mask to save registers C clobbers d0,d1,a0,a1
CREST	= 0x0303	| mask to restore registers C clobbers
CSIZE	= 16		| size of saved registers on stack

| size of silo for char input buffer
SILO = 4096

| built in trap types
SCHEDULE= 256
STARTIT	= 255


| Reset vectors are in first text page
| SDU fills in reset sp
| The vector stuff is swiped from TI since the code
|   is the only documentation for the interrupt stuff


	.text

|Priority Level 0, Vectors 0 - 31

reset:	.long	0		|0	reset sp - initialized by uboot
	.long	_start		|1	reset pc
	.long	buserr		|2
	.long	addrerr		|3

	.long	illegal		|4
	.long	zerodiv		|5
	.long	chkinst		|6
	.long	trapvinst	|7

	.long	priviledge	|8
	.long	trace		|9
	.long	line1010	|10
	.long	line1111	|11

	.long	badintr
	.long	badintr
	.long	formaterr	|14
	.long	uninitvec	|15

	|16-31 Not recognized by kernel, generally reserved to motorola
	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	spurious	|24
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

|Priority Level 1, Vectors 32 - 63

	|32-47 TRAP Invstruction Vectors
	.long	trap0		|32
	.long	trap1		|33
	.long	trap2		|34
	.long	trap3		|35

	.long	trap4		|36
	.long	trap5		|37
	.long	trap6		|38
	.long	trap7		|39

	.long	trap8		|40
	.long	trap9		|41
	.long	trap10		|42
	.long	trap11		|43

	.long	trap12		|44
	.long	trap13		|45
	.long	trap14		|46
	.long	trap15		|47

	|48-63 (Unassigned, Reserved)
	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr
	
|Priority Level 2, Vectors 64 - 95

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

|Priority Level 3, Vectors 96 - 127
|This is the maximum level that can be used for wakeups

	| low priority async receive interrupts (can actually do wakeup)
ASH0LR = 96*4
	.long	ash0lr
|	.long	badintr
ASH1LR = 97*4
	.long	ash1lr
|	.long	badintr
	.globl	_ashx
_ashx:	| low priority async transmit interrupts (can do it all)
	.long	ash0x
|	.long	badintr
	.long	ash1x
|	.long	badintr

|.set	_PEGWK,	100*4 + _intrs
	.long	pegwk
|	.long	badintr
	.globl	_evec
_evec:	.long	ethhnl
|	.long	badintr
	.globl	_smdintv
_smdintv:| disk interrupt
	.long	smdintr
|	.long	badintr
	.globl _clklow
CLKL = 4*103
_clklow: .long  clklint
|	.long	badintr

|.set	_DOSCHED, 104*4 + _intrs
	.long	dosched
|	.long	badintr

	.globl	_qtr
_qtr:	.long	qtrhnl
|	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

|Priority Level 4, Vectors 128 - 159

	.globl	_ashr
_ashr:	| high priority async receive interrupt (just puts data in silo)
	.long	ash0r
|	.long	badintr
	.long	ash1r
|	.long	badintr
	.globl	_peg
_peg:	| high priority async receive interrupt (polls mouse and keyboard)
	.long	peg0r
|	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

|Priority Level 5, Vectors 160 - 191

	.globl	_clk
_clk:	.long	clkhnl
|	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

|Priorty Level 6, Vectors 192 - 223

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

|Priority Level 7, Vectors 224 - 255

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

	.long	badintr
	.long	badintr
	.long	badintr
	.long	badintr

|End interrupt vectors


|First page of data is page map set by SDU??

	.data

	.globl	_Sysmap
_Sysmap:	.=256*4+.		| the kernel can be 256k

| video memory and control registers
| moving the video addresses requires some user programs to be rebuilt
	.globl	_vcconmap, _vcsltmap, _vcrammap, _vcrommap
_vcconmap:	.=1*4+.
	.set	_vccon, _vcconmap-_Sysmap*256+SYSTEM
_vcsltmap:	.=8*4+.
	.set	_vcslt, _vcsltmap-_Sysmap*256+SYSTEM
_vcrammap:	.=128*4+.
	.set	_vcram, _vcrammap-_Sysmap*256+SYSTEM
_vcrommap:	.=4+.
	.set	_vcrom, _vcrommap-_Sysmap*256+SYSTEM

| temporary map locations for accessing otherwise unmapped segments
	.globl	_tmp1map, _tmp2map, _tmp3map, _tmp4map, _tmprmap
_tmp1map:	.=1*4+.
	.set	_tmp1, _tmp1map-_Sysmap*256+SYSTEM
_tmp2map:	.=1*4+.
	.set	_tmp2, _tmp2map-_Sysmap*256+SYSTEM
_tmp3map:	.=1*4+.
	.set	_tmp3, _tmp3map-_Sysmap*256+SYSTEM
_tmp4map:	.=1*4+.
	.set	_tmp4, _tmp4map-_Sysmap*256+SYSTEM
_tmprmap:	.=1*4+.
	.set	_tmpr, _tmprmap-_Sysmap*256+SYSTEM

| asynchronous I/O ports
	.globl	_ashmap
_ashmap:	.=1*4+.
	.set	_asy, _ashmap-_Sysmap*256+SYSTEM

| 8k for 3Com multibus ethernet controller
	.globl	_mecmap
_mecmap:	.=8*4+.
	.set	_mec, _mecmap-_Sysmap*256+SYSTEM

| smd controller
	.globl	_smdmap
_smdmap:	.=1*4+.	
	.set	_smd, _smdmap-_Sysmap*256+SYSTEM

| cmos configuration ram
	.globl	_cmosmap
_cmosmap:	.=8*4+.
	.set	_cmos, _cmosmap-_Sysmap*256+SYSTEM

| sdu multibus -> nubus map
	.globl	_multimap
_multimap:	.=4*4+.	
	.set	_multi, _multimap-_Sysmap*256+SYSTEM

| quarter track tape map
	.globl	_qtrmap
_qtrmap:	.=2*4+.
	.set	_qtrizat, _qtrmap-_Sysmap*256+SYSTEM

	.globl	_cdatamap
_cdatamap:	.=8*4+.
	.set	_cdata, _cdatamap-_Sysmap*256+SYSTEM
	.set	_tlb2, _cdata+0x1000
	.set	_usrpbr, _cdata+0x1900		|used by resume
	.set	_syspbr, _usrpbr+4		|used by resume

	.globl	_ctagsmap
_ctagsmap:	.=8*4+.
	.set	_ctags, _ctagsmap-_Sysmap*256+SYSTEM
	.set	_tlb2tags, _ctags+0x1000

| interrupt control registers on CPU board
	.globl	_intrmap
_intrmap:	.=1*4+.
	.set	_intrs, _intrmap-_Sysmap*256+SYSTEM
	.set	_PEGWK,	100*4 + _intrs
	.set	_DOSCHED, 104*4 + _intrs

	.globl	_cpu1map
_cpu1map:	.=1*4+.
	.set	_cpuctl, _cpu1map-_Sysmap*256+SYSTEM
	.set	_syscid, _cpuctl+4		|used by resume
	.set	_usrcid, _syscid+4		|used by resume
	.set	_cachectl, _usrcid+4		|used by resume

	.globl	_cpu2map
_cpu2map:	.=1*4+.
	.set	_cpuconf, _cpu2map-_Sysmap*256+SYSTEM

	.globl	_mnc1map
_mnc1map:	.=1*4+.
	.set	_mncram, _mnc1map-_Sysmap*256+SYSTEM

	.globl	_mnc2map
_mnc2map:	.=4+.
	.set	_mnccfg, _mnc2map-_Sysmap*256+SYSTEM

| physical memory is mapped in here for applications requiring dma
	.globl	_uncachmap
_uncachmap:	.=32*4+.
	.set	_uncached, _uncachmap-_Sysmap*256+SYSTEM


| The second page of text is where execution begins

	.text
	.globl	_start, _main, _initmap, _cpu_type

_start:					| determine cpu_type
	movl	#68000, d0		| trap on 68000
	movl	#-1, _cpu_type		| 
	.word	0x42c0			| try 68010: move from ccr
	tstl	_cpu_type		| did we find cpu type?
	bgt	1$
	movl	#68010, _cpu_type	| default to 68010
1$:
	movl	sp, _initmap
	movl	#_kstop, sp
	jsr	_main
	jra	didtrap

	.globl	_mpanic
uhoh:
	jsr	_mpanic		| should never return to here
	bra	.


|Validate just make sure that the appropriate level 2 pte entry will
|get reloaded next time it is used.  Note that we do not flush the
|data cache so this had better be a no cache page.
|This is straight from TI for the time being

	.globl	_validate
_validate:
	movl	sp@(4),d0	|the virtual address to validate
	lsrl	#PGSHIFT-2,d0	|convert vaddr to tlb2 index 
	andl	#L2MASK,d0	|modulo size of tlb2
	movl	#_tlb2tags,a0	|get a pointer to tlb2tags
	movl	#_tlb2,a1	|and one to tlb2 data
	addl	d0,a0		|adjust pointer to proper entry
	addl	d0,a1		|do it to this pointer too
	movw	sr,d0		|save it
	movw	#HIGH,sr	|don't bug me now
	andb	#NOTLB,_cachectl	|turn off translation buffers
	movl	#0,a0@		|zero the tags to set cid to 0
	movl	#0,a1@		|zero the data to set parity properly in tags
	orb	#DOTLB,_cachectl	|now enable translation again
	movw	d0,sr		|resume normal priority
	rts



| this version of validate can only be called to validate a page in the
| system domain address space

	.globl	_uvalidate
_uvalidate:
	movl	sp@(4),d0	|the virtual address to validate
	lsrl	#PGSHIFT-2,d0	|convert vaddr to tlb2 index 
	andl	#L2MASK,d0	|modulo size of tlb2
	movl	#_tlb2tags,a0	|get a pointer to tlb2tags
	movl	#_tlb2,a1	|and one to tlb2 data
	addl	d0,a0		|adjust pointer to proper entry
	addl	d0,a1		|do it to this pointer too
| the argument for why this version doesn't need to lock out interrupts
| depends upon the fact that there is only one page in the system domain that
| can map to any level2 pte (thus we can be careful that no interrupting
| routine will effect this entry)
| if in the future this is not the case an spl trap will be necessary
| or alternatively we could simply force some other page to be loaded
| by touching the virtual address+512K
	andb	#NOTLB,_cachectl	|turn off translation buffers
	movl	#0,a0@		|zero the tags to set cid to 0
	movl	#0,a1@		|zero the data to set parity properly in tags
	orb	#DOTLB,_cachectl	|now enable translation again
	rts


|resume is hard because I cannot touch stack while doing it
|for details on how resume flushes the cache, see cflush below

	.globl	_resume
_resume:
	moveml	#0xF8E0,sp@-	| d0-d4 a0-a2
	movw	sr,d0		|save me a copy of status
	movl	_syspbr,_usrpbr	|install map in pbr's

	movl	#SYSTEM,a2	|get ready to start touching ram locations
	clrl	d1
	movb	cid,d1
	subl	#2,d1
	lsll	#2,d1
	addl	d1,a2
	movl	#4*1024+SYSTEM,d2 |stop here
	movb	#1,_syscid	|we are going to cache flush to 0 as our cid

1$:	cmpl	a2,d2
	ble	3$
	movb	a2@,d4		|just fetch to set this cache cid to 0
	addl	#62*4,a2	|bump to next cache entry
	bra	1$
3$:

	movl	#_tlb2+16,a2	|pointer to tlb2 registers on cpu board
	movl	#_tlb2tags+16,a1	|pointer to tlb2 tags on cpu board
	movl	a2,d2		|number of entries in TLB2 and TLB1
	addl	#4*571,d2
	addl	d1,a2
	addl	d1,a1
	moveq	#0,d1		|we're going to fill buffers with 0
2$:	cmpl	a2,d2
	ble	5$

	movw	#HIGH,sr	|don't bother me until I switch registers
	andb	#NOTLB,_cachectl	|turn off translation buffers
	movl	d1,a1@		|zero the tags to set cid to 0
	movl	d1,a2@		|zero the data to set parity properly in tags
	orb	#DOTLB,_cachectl	|now enable translation again
	movw	d0,sr		|spl 0,	supervisor mode
	addl	#62*4,a1
	addl	#62*4,a2
	bra	2$

5$:
	addb	#1,cid		|advance to next cid
	cmpb	#64,cid
	bne	4$
	movb	#2,cid
4$:	movb	cid,_syscid	|install new system cid
	movb	cid,_usrcid	|install new usr cid
	moveml	sp@+,#0x071F
	moveq	#1,d0		|return 1
	rts


|Test and set a designated location return condition code values

	.globl	_tas
_tas:
	movl	sp@(8),d1	|delay count in iterations
	movl	sp@(4),a0	|argument is a byte address to tas
1$:	movw	sr,d0		|get priority like an spl
	movl	#HIGH,sr
	tas	a0@		|do it
	jne	2$		|whoops, somebody already has it
	rts

2$:	movw	d0,sr		|let anther interrupts happen for a moment
	jsr	_delay		|do nothing for 100 microseconds 
	subql	#1,d1		|decrement
	jge	1$		|ok, let's try again
	movl	#-1,d0		|signal caller we failed
	rts			|give up with -1 error return


| Do nothing for a while without touching the bus

	.globl	_delay
_delay:
	movw	#100,d0
	dbf	d0,.
	rts


| blink the cpu board led

	.globl	_led_on, _led_off, _led_xor
_led_on:
	movb	0xf6e400, d0
	orb	#4, d0
	movb	d0, 0xf6e400
	rts

_led_off:
	movb	0xf6e400, d0
	andb	#0xFB, d0
	movb	d0, 0xf6e400
	rts

_led_xor:
	movb	0xf6e400, d0
	eorb	#4, d0
	movb	d0, 0xf6e400
	rts


| set priority levels

	.globl	_hpl7, _hpl6, _hpl5, _hpl1, _hpl0, _hplx
_hpl7:
	movw	sr,d0
	movw	#0x2700,sr
	rts

_hpl6:
	movw	sr,d0
	movw	#0x2600,sr
	rts

_hpl5:
	movw	sr,d0
	movw	#0x2500,sr
	rts

_hpl1:
	movw	sr,d0
	movw	#0x2100,sr
	rts

_hpl0:
	movw	sr,d0
	movw	#0x2000,sr
	rts

_hplx:
	movw	sr,d0		|return current priority
	movw	sp@(6),sr	|the priority he passed as arg
	rts


	.text

| bus error and address error on the 68000 leave the stack somewhat screwed
| we move everything to make it look like the 68010
| note that everytime one of these happens on the 68000 50 words of kernel
|   stack are lost

buserr:
	cmpl	#68000, _cpu_type
	bne	1$
	subl	#42,sp		| convert to an extended (68010) frame
	movw	sp@(50),sp@	| move psw into place
	movl	sp@(52),sp@(2)	| move pc into place
	movw	#0x8000,sp@(6)	| simulate format word (vector not supported)
	movw	sp@(42),sp@(8)	| move fcode to special status word
	movl	sp@(44),sp@(10)	| move aaddr to fault address
	movw	sp@(48),sp@(24)	| move ireg to instruction input buffer
1$:
	movw	#2,sp@-		|trap type 2
	jra	fault

addrerr:
	cmpl	#68000, _cpu_type
	bne	1$
	subl	#42,sp		| convert to an extended (68010) frame
	movw	sp@(50),sp@	| move psw into place
	movl	sp@(52),sp@(2)	| move pc into place
	movw	#0x8000,sp@(6)	| simulate format word (vector not supported)
	movw	sp@(42),sp@(8)	| move fcode to special status word
	movl	sp@(44),sp@(10)	| move aaddr to fault address
	movw	sp@(48),sp@(24)	| move ireg to instruction input buffer
1$:
	movw	#3,sp@-		|trap type 3
	jra	fault


|the actual entry points push a trap number and jmp to trap

illegal:
	tstl	_cpu_type	| if negative, we are finding cpu type
	jge	zerodiv		| continue processing a normal trap
	movl	d0,_cpu_type	| instruction failed --> cpu type in d0
	addql	#2,sp@(2)	| skip bad instruction
	rte
zerodiv:
chkinst:
trapvinst:
line1010:
line1111:
priviledge:
spurious:
formaterr:
uninitvec:
trap9:
trap10:
trap11:
trap12:
trap13:
trap14:
trap15:
	movw	#4,sp@-
	jra	fault

trap0:
	movw	#32,sp@-	| kernal call
	jra	fault

trap1:
	movw	#33,sp@-	| traceon
	jra	fault
trap2:
	movw	#34,sp@-	| traceoff
	jra	fault

trap3:
	movw	#35,sp@-	| reschedule
	jra	fault

trap4:
	movw	#36,sp@-	| debugon
	jra	fault
trap5:
	movw	#37,sp@-	| debugoff
	jra	fault

trap6:
	movw	#38,sp@-	| profon
	jra	fault
trap7:
	movw	#39,sp@-	| profoff
	jra	fault


	.globl	_D_CURRENT, _D_SYSTEM, _splpanic
trap8:				| TRIX spl trap
	movw	sp@,d0		| d0 = interrupt ps
	andl	#SMODE,d0	| in kernel mode?
	bne	1$		|	yes - set priority level
	movl	_D_SYSTEM,d0	| d0 = kernel domain pointer
	cmpl	_D_CURRENT,d0	| are we in the kernel domain?
	beq	1$		|	yes - set priority level
	jsr	_splpanic
	moveq	#-1,d0		| d0 = -1
	rte
1$:
	andl	#PRIMSK,d1	| only set priority bits
	movw	sp@,d0
	andl	#SMODE,d0	| yup, get the SMODE bit again
	orl	d0,d1		| use old KERNEL bit
	movw	sp@,d0		| return old ps value
	andl	#PRIMSK,d0	| only return priority bits
	movw	d1,sp@
	rte

trace:
	movw	#9,sp@-		| trace trap
	jra	fault


	.globl	_spl7, _spl0, _splx
_spl7:
	movl	#PRI7,d1
	trap	#8
	rts

_spl0:
	movl	#PRI0,d1
	trap	#8
	rts

_splx:
	movl	sp@(4),d1
	trap	#8
	rts


	.text

	.globl	_trap, _ktrap, _dosched
	.globl	_T_CURRENT

|  fault is called from an interrupt or exception:
|  fault saves the registers in the thread if the call is from umode
|    and calls the c routine trap() or ktrap()
|  the form of the calls are:
|    trap(number, padding, ups, upc)
|    ktrap(number, d0, d1, a0, a1, padding, ups, upc)
|
|  after trap returns the registers are restored from the thread if the
|  return is to umode

fault:				| sp long aligned (psw is a short) by number
	btst	#SBITNUM,sp@(2)	| test SMODE bit
	bne	kfault

dofault:
	movl	a0,sp@-
	movl	_T_CURRENT,a0
	moveml	#0x7FFF,a0@(4)	| save registers d0-d7 a0-a6 in thread struct
	movl	sp@,a0@(36)	| save real a0 in thread struct
	movl	usp,a1		| a1 is ok, the registers are already saved
	movl	a1,a0@(64)	| save usp in thread struct
	addql	#4,sp		| throw away saved a0 on stack

	clrl	d0
	movw	sp@+,d0		| trap number (high word of aligned psw)

	movw	sp@+,a0@(68)	| save fualting psw
	movl	sp@+,a0@(70)	| save fualting pc
	movw	sp@+,d1
	movw	d1,a0@(74)	| trap type and vector offset	
	andl	#0xF000,d1	| mask out vector offset
	beq	2$		| short fault

	moveml	sp@+,#0x7E7E	| long fault: get 24 of the 25 extra words
	moveml	#0x7E7E,a0@(76)	| save them in thread structure
	movw	sp@+,a0@(124)	| save the last of the extra words

2$:	movl	d0,sp@-		| push fault number
	jsr	_trap		| C handler for traps and faults
	addql	#4,sp		| pop fault number

didtrap:
	movl	_T_CURRENT,a0
	movw	a0@(74),d0	| trap type and vector offset
	andl	#0xF000,d0	| mask out vector offset
	beq	4$		| short fault

	movw	a0@(124),sp@-	| long fault: save the last of the extra words
	moveml	a0@(76),#0x7E7E	| get 24 0f the 25 extra words
	moveml	#0x7E7E,sp@-	| save them in the thread structure

4$:	movw	a0@(74),sp@-	| restore trap type and vector offset
	movl	a0@(70),sp@-	| restore faulting pc
	movw	a0@(68),sp@-	| restore faultint psw

	movl	a0@(64),a1
	movl	a1,usp		| restore usp from thread struct
	moveml	a0@(4),#0x7FFF	| restore registers d0-d7 a0-a6 from thread
	rte

| deal with a fault from kernel mode

kfault:
	moveml	#CSAVE,sp@-	| push d0-d1 a0-a1

	clrl	d0
	movw	sp@(CSIZE),d0	| trap number (high word of aligned psw)

	movl	d0,sp@-		| push fault number
	jsr	_ktrap		| C handler for traps and faults
	addql	#4,sp		| pop fault number

	moveml	sp@+,#CREST	| first restore d0-d1 a0-a1 (leave ccodes)
	addql	#2,sp		| pop alignment word
	rte


| int_rte is called after processing an interrupt with C registers saved
| it checks dosched and calls trap if necessary

int_rte:
	moveml	sp@+,#CREST	| restore C registers
	btst	#SBITNUM,sp@(2)	| test SMODE bit
	jne	1$		|   yes, just return
	tstl	_dosched	| time to reschedule
	jne	dofault		| continue handling the SCHEDULE fault
1$:
	addql	#2,sp		| pop alignment word
	rte


	.globl	_printint
badintr:
	subl	#2,sp		| align ps
	moveml	#CSAVE,sp@-
	jsr	_printint
	moveml	sp@+,#CREST
	addql	#2,sp		| pop off jsr pc and alignment word
	rte			| and hope for the best


| Some good interrupt handlers

	.globl	_pci_rint, _pci_xint
	.globl	_silo0r, _silo1r
	.globl	_silo0tim, _silo1tim

ash0r:
	moveml	#CSAVE,sp@-	| save registers that C clobbers
	movl	_silo0r, a0	| in pointer
	movl	_silo0r+8, a1	| device register
	movb	a1@, a0@+	| put byte in silo
	cmpl	_silo0r+4, a0	| will it overflow the silo?
	beq	2$
	cmpl	#_silo0r+12+SILO, a0	| wrap around pointer?
	bne	1$
	movl	#_silo0r+12, a0	| wrap it
1$:
	movl	a0, _silo0r	| it fit so bump the silo pointer
		| could forstall interrupt till a high water mark, but...
	tstl	_silo0tim
	bne	2$
	movb	#1, _intrs+ASH0LR	| force low priority interrupt
2$:
	moveml	sp@+,#CREST	| restor C registers
	rte			| return from interrupt

ash0lr:
	movw	#SCHEDULE,sp@-	| align psw with SCHEDULE trap number
	moveml	#CSAVE,sp@-	| save registers that C clobbers
	movl	#0,sp@-		| receiver interrupt on port 0
	jsr	_pci_rint	| call handler
	addql	#4,sp
	jra	int_rte


ash0x:
	movw	#SCHEDULE,sp@-
	moveml	#CSAVE,sp@-
	movl	#0,sp@-
	jsr	_pci_xint	| call handler
	addql	#4,sp
	jra	int_rte


ash1r:
	moveml	#CSAVE,sp@-
	movl	_silo1r, a0	| in pointer
	movl	_silo1r+8, a1	| device register
	movb	a1@, a0@+	| put byte in silo
	cmpl	_silo1r+4, a0	| will it overflow the silo?
	beq	2$
	cmpl	#_silo1r+12+SILO, a0	| wrap around pointer?
	bne	1$
	movl	#_silo1r+12, a0	| wrap it
1$:
	movl	a0, _silo1r	| it fit so bump the silo pointer
		| could forstall interrupt till a high water mark, but...
	tstl	_silo1tim
	bne	2$
	movb	#1, _intrs+ASH1LR	| force low priority interrupt
2$:
	moveml	sp@+,#CREST	| restor C registers
	rte			| return from interrupt

ash1lr:
	movw	#SCHEDULE,sp@-
	moveml	#CSAVE,sp@-
	movl	#1,sp@-
	jsr	_pci_rint	| call handler
	addql	#4,sp
	jra	int_rte


ash1x:
	movw	#SCHEDULE,sp@-
	moveml	#CSAVE,sp@-
	movl	#1,sp@-
	jsr	_pci_xint	| call handler
	addql	#4,sp
	jra	int_rte

	.globl _tim_int
clklint:
	movw	#SCHEDULE,sp@-
	moveml	#CSAVE,sp@-
	movl	#1,sp@-

	tstl	_silo0tim
	beq	$0
	subql	#1, _silo0tim
	bne	$0
	movb	#1, _intrs+ASH0LR	| force low priority interrupt
$0:	tstl	_silo1tim
	beq	$1
	subql	#1, _silo1tim
	bne	$1
	movb	#1, _intrs+ASH1LR	| force low priority interrupt

$1:	jsr	_tim_int	| call handler
	addql	#4,sp
	jra	int_rte


	.globl	_pegintr, _pegwakeup
peg0r:
	movw	#SCHEDULE,sp@-
	moveml	#CSAVE,sp@-
	movl	#0,sp@-
	jsr	_pegintr	| call handler
	addql	#4,sp
	jra	int_rte
pegwk:
	movw	#SCHEDULE,sp@-
	moveml	#CSAVE,sp@-
	movl	#0,sp@-
	jsr	_pegwakeup	| call handler
	addql	#4,sp
	jra	int_rte

dosched:
	movw	#SCHEDULE,sp@-
	moveml	#CSAVE,sp@-
	movl	#1,_dosched	| just set "real" dosched
	jra	int_rte

	.globl	_eth_int
ethhnl:
	movw	#SCHEDULE,sp@-
	moveml	#CSAVE,sp@-
	movl	#0,sp@-
	jsr	_eth_int	| call handler
	addql	#4,sp
	jra	int_rte

	.globl	_qtr_int
qtrhnl:
	movw	#SCHEDULE,sp@-
	moveml	#CSAVE,sp@-
	movl	#0,sp@-
	jsr	_qtr_int	| call handler
	addql	#4,sp
	jra	int_rte

	.globl	_clk_rint
clkhnl:
	movw	#SCHEDULE,sp@-
	moveml	#CSAVE,sp@-
	movl	#0,sp@-
	jsr	_clk_rint
	movb	#1, _intrs+CLKL	| force low priority interrupt
	addql	#4,sp
	jra	int_rte

	.globl	_smd_int
smdintr:
	movw	#SCHEDULE,sp@-
	moveml	#CSAVE,sp@-
	movl	#0,sp@-
	jsr	_smd_int
	addql	#4,sp
	jra	int_rte


| This should go someplace else
	.data
cid:
	.byte 2
	.even
_cpu_type:
	.long	68000		| if we dont change it, its a 68000


	.globl	_kstack, _kstop

_kstack:
	.zerol	4096		| space for kernel stack
_kstop:
