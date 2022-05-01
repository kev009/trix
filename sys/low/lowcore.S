
|	(C) COPYRIGHT, TEXAS INSTRUMENTS INCORPORATED, 1983.  ALL
|	RIGHTS RESERVED.  PROPERTY OF TEXAS INSTRUMENTS INCORPORATED.
|	RESTRICTED RIGHTS - USE, DUPLICATION, OR DISCLOSURE IS SUBJECT
|	TO RESTRICTIONS SET FORTH IN TI'S PROGRAM LICENSE AGREEMENT AND
|	ASSOCIATED DOCUMENTATION.


|	sccsid[] = "@(#)locore.s	1.14 (Texas Instruments) 83/08/24"

|low core for text and data segments
|Bread board version to support		async ports on sdu. 3/15/83
NBPG=1024
NPTEPG=256
SYSTEM=0xF00000			|address where system starts
HIGH=0x2700			|high priority for status register
NOTLB=0x80+0x40+0x2+0x1		|cache enable with translation and parity
ENTLB=NOTLB+0x20+0x10+0x8+0x4	|enable above plus translation buffers
MAXCID=63
PGSHIFT=10			|log2(NBPG)
L2MASK=2048-1			|mask for mod (TLB2SIZE << 2)
SHIFT=0x1			|multiplier used with trap entries
UPAGES=7			|number of pages of usrtbl stuff
USTUFF=UPAGES*NBPG		|usrtbl stuff in bytes
L1SIZE=64*4			|number of significant entries in L1 map
CSAVE=0xD0C0			|moveml sp@- mask for registers C uses
CREST=0x30B			|moveml sp@+ mask for registers C uses
NOTLB=0xC3			|"and" mask to disable tlb2 and tlb1
DOTLB=0x3C			|"or" mask to enable tlb2 and tlb1
WPAGES=24			|pages of window system buffers

|layout UPAGES
	.set	u,SYSTEM-USTUFF+L1SIZE


|Priority Level 0, Vectors 0 - 31

	.text
reset:	.long	0		|0	reset sp - initialized by boot program
	.long	start		|1	reset pc
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
	.long	badvec+72
	.long	badvec+78
	.long	badvec+84
	.long	badvec+90

|16-31 Not recognized by kernel, generally reserved to motorola
	.long	badvec+96
	.long	badvec+102
	.long	badvec+108
	.long	badvec+114

	.long	badvec+120
	.long	badvec+126
	.long	badvec+132
	.long	badvec+138

	.long	badvec+144
	.long	badvec+150
	.long	badvec+156
	.long	badvec+162

	.long	badvec+168
	.long	badvec+174
	.long	badvec+180
	.long	badvec+186


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
	.long	badvec+288
	.long	badvec+294
	.long	badvec+300
	.long	badvec+306

	.long	badvec+312
	.long	badvec+318
	.long	badvec+324
	.long	badvec+330

	.long	badvec+336
	.long	badvec+342
	.long	badvec+348
	.long	badvec+354

	.long	badvec+360
	.long	badvec+366
	.long	badvec+372
	.long	badvec+378
	

|Priority Level 2, Vectors 64 - 95

| 64-79 Should never happen
	.long	badvec+384
	.long	badvec+390
	.long	badvec+396
	.long	badvec+402

	.long	badvec+408
	.long	badvec+414
	.long	badvec+420
	.long	badvec+426

	.long	badvec+432
	.long	badvec+438
	.long	badvec+444
	.long	badvec+450

	.long	badvec+456
	.long	badvec+462
	.long	badvec+468
	.long	badvec+474


|80 - 95 should never happen 
	.long	badvec+480
	.long	badvec+486
	.long	badvec+492
	.long	badvec+498

	.long	badvec+504
	.long	badvec+510
	.long	badvec+516
	.long	badvec+522

	.long	badvec+528
	.long	badvec+534
	.long	badvec+540
	.long	badvec+546

	.long	badvec+552
	.long	badvec+558
	.long	badvec+564
	.long	badvec+570
	

|Priority Level 3, Vectors 96 - 127

|96 - 99 are for sdu's serial ports
| rcv & xmt for each
	.globl  ash
ash:	.long	ash0r	
	.long	ash0x
	.long	ash1r
	.long	ash1x

|100 - 103 should never happen
	.long	badvec+594
	.long	badvec+600
	.long	badvec+606
	.long	badvec+612

| 104 assigned to:
| MTI-800 interrupt vector: only one.
	.globl	mtivec
mtivec:	.long	mtihndl

| 105 & 106 assigned to:
| CDC octal serial interface vector
	.globl	osivec
osivec:	.long	osirrpt
	.long	ositrpt

| 107 assigned to vcmem
	.globl	vcvec
vcvec:	.long	vchndl

|108-111 should never happen
	.long	badvec+630
	.long	badvec+636
	.long	badvec+642
	.long	badvec+648

|112-127 should never happen
	.long	badvec+654
	.long	badvec+660
	.long	badvec+666
	.long	badvec+672

	.long	badvec+678
	.long	badvec+684
	.long	badvec+690
	.long	badvec+696

	.long	badvec+702
	.long	badvec+708
	.long	badvec+714
	.long	badvec+720

	.long	badvec+726
	.long	badvec+732
	.long	badvec+738
	.long	badvec+744


|Priority Level 4, Vectors 128 - 159

|128-143 should never happen
	.long	badvec+750
	.long	badvec+756
	.long	badvec+762
	.long	badvec+768

	.long	badvec+774
	.long	badvec+780
	.long	badvec+786
	.long	badvec+792

	.long	badvec+798
	.long	badvec+804
	.long	badvec+810
	.long	badvec+816

	.long	badvec+822
	.long	badvec+828
	.long	badvec+834
	.long	badvec+840

|144-159 should never happen
	.long	badvec+846
	.long	badvec+852
	.long	badvec+858
	.long	badvec+864

	.long	badvec+870
	.long	badvec+876
	.long	badvec+882
	.long	badvec+888

	.long	badvec+894
	.long	badvec+900
	.long	badvec+906
	.long	badvec+912

	.long	badvec+918
	.long	badvec+924
	.long	badvec+930
	.long	badvec+936


|Priority Level 5, Vectors 160-191
	
	.globl	sdu
sdu:	.long	sdu0
	.long	sdu1
	.long	sdu2
	.long	sdu3

	.long	sdu4
	.long	sdu5
	.long	sdu6
	.long	sdu7

	.long	sdu8
	.long	sdu9
	.long	sdu10
	.long	sdu11

	.long	sdu12
	.long	sdu13
	.long	sdu14
	.long	sdu15


	.globl	qtr
qtr:	.long	qtrhndl

	.globl  smdvec
smdvec: .long	smdhndl

	.globl	tmvec
tmvec:	.long	tmhndl

 	.globl	ethrvec
ethrvec:.long	ethrhndl


 	.globl	ethr2vec
ethr2vec:.long	ethr2hndl

|181-191 should never happen

	.long	badvec+1044
	.long	badvec+1050
	.long	badvec+1056

	.long	badvec+1062
	.long	badvec+1068
	.long	badvec+1074
	.long	badvec+1080

	.long	badvec+1086
	.long	badvec+1092
	.long	badvec+1098
	.long	badvec+1104


|Priorty Level 6, Vectors 192 - 223

	.globl	clk
clk:	.long	clkhndl

|Interrupts  193-207 should never happen
	.long	badvec+1110
	.long	badvec+1116
	.long	badvec+1122

	.long	badvec+1128
	.long	badvec+1134
	.long	badvec+1140
	.long	badvec+1146

	.long	badvec+1152
	.long	badvec+1158
	.long	badvec+1164
	.long	badvec+1170

	.long	badvec+1176
	.long	badvec+1182
	.long	badvec+1188
	.long	badvec+1194

|208-223 should never happen
	.long	badvec+1200
	.long	badvec+1206
	.long	badvec+1212
	.long	badvec+1218

	.long	badvec+1224
	.long	badvec+1230
	.long	badvec+1236
	.long	badvec+1242

	.long	badvec+1248
	.long	badvec+1254
	.long	badvec+1260
	.long	badvec+1266

	.long	badvec+1272
	.long	badvec+1278
	.long	badvec+1284
	.long	badvec+1290


|Priority Level 7, Vectors 224 - 255

|224-239 should never happen
	.long	badvec+1296
	.long	badvec+1302
	.long	badvec+1308
	.long	badvec+1314

	.long	badvec+1320
	.long	badvec+1326
	.long	badvec+1332
	.long	badvec+1338

	.long	badvec+1344
	.long	badvec+1350
	.long	badvec+1356
	.long	badvec+1362

	.long	badvec+1368
	.long	badvec+1374
	.long	badvec+1380
	.long	badvec+1386

|240-255 should never happen
	.long	badvec+1392
	.long	badvec+1398
	.long	badvec+1404
	.long	badvec+1410

	.long	badvec+1416
	.long	badvec+1422
	.long	badvec+1428
	.long	badvec+1434

	.long	badvec+1440
	.long	badvec+1446
	.long	badvec+1452
	.long	badvec+1458

	.long	badvec+1464
	.long	badvec+1470
	.long	badvec+1476
	.long	badvec+1482

|End interrupt vectors

|Low page of data is system map
	.data
	.globl	Sysmap

Sysmap:	.=200*4+.	|map of system text and data

	.globl bufmap
bufmap:	.=65*4+.	|this should be >= NBUF
	.set	buffers,bufmap-Sysmap*NPTEPG+SYSTEM

	.globl miomap
miomap:	.=1*4+.	
	.set	mio,miomap-Sysmap*NPTEPG+SYSTEM

	.globl mmmap;
mmmap:	.=4*4+.	
	.set	multmap,mmmap-Sysmap*NPTEPG+SYSTEM

	.globl ashmap
ashmap:	.=1*4+.
	.set	asy,ashmap-Sysmap*NPTEPG+SYSTEM

	.globl osimap	| map in 16k onboard memory on CDC board
osimap:	.=16*4+.
	.set	osi,osimap-Sysmap*NPTEPG+SYSTEM

	.globl enetmap	| map in 8k for ethernet controller
enetmap:.=8*4+.
	.set	enet,enetmap-Sysmap*NPTEPG+SYSTEM

	.globl enet2map	| map in 8k for ethernet controller
enet2map:.=8*4+.
	.set	enet2,enet2map-Sysmap*NPTEPG+SYSTEM

	.globl qtrmap	| map in 2k for data area and registers
qtrmap:	.=2*4+.
	.set	qtrizat,qtrmap-Sysmap*NPTEPG+SYSTEM

	.globl qmmap	| map in 2k for qtr's byte transfer to/from user area
qmmap:	.=2*4+.
	.set	qmmizat, qmmap-Sysmap*NPTEPG+SYSTEM

	.globl tmmap	| map in Tapemaster System Configuration Pointer
tmmap:	.=1*4+.
	.set	tm,tmmap-Sysmap*NPTEPG+SYSTEM

	.globl	cmosmap	| map in configuration ram
cmosmap:.=8*4+.
	.set cmos, cmosmap-Sysmap*NPTEPG+SYSTEM

	.globl cdatamap
cdatamap:
	.=8*4+.
	.set	cdata,cdatamap-Sysmap*NPTEPG+SYSTEM
	.set	tlb2,cdata+0x1000
	.set	usrpbr,cdata+0x1900	|used by resume
	.set	syspbr,usrpbr+4		|used by resume

	.globl ctagsmap
ctagsmap:
	.=8*4+.
	.set	ctags,ctagsmap-Sysmap*NPTEPG+SYSTEM
	.set	tlb2tags,ctags+0x1000

	.globl cpu1map
cpu1map:
	.=4+.
	.set	cpuctl,cpu1map-Sysmap*NPTEPG+SYSTEM
	.set	syscid,cpuctl+4		|used by resume
	.set	usrcid,syscid+4		|used by resume
	.set	cachectl,usrcid+4	|used by resume

	.globl cpu2map
cpu2map:
	.=4+.
	.set	cpucfg,cpu2map-Sysmap*NPTEPG+SYSTEM

	.globl mnc1map
mnc1map:
	.=4+.
	.set	mncram,mnc1map-Sysmap*NPTEPG+SYSTEM

	.globl mnc2map
mnc2map:
	.=4+.
	.set	mnccfg,mnc2map-Sysmap*NPTEPG+SYSTEM

	.globl tmp1map
tmp1map:
	.=4+.
	.set	tmp1,tmp1map-Sysmap*NPTEPG+SYSTEM

	.globl tmp2map
tmp2map:
	.=4+.
	.set	tmp2,tmp2map-Sysmap*NPTEPG+SYSTEM

	.globl	vcconmap, vcsltmap, vcrammap, vcrommap, wbufmap
vcconmap:
	.=4+.
	.set	vccon,vcconmap-Sysmap*NPTEPG+SYSTEM
vcsltmap:
	.=8*4+.
	.set	vcslt,vcsltmap-Sysmap*NPTEPG+SYSTEM
vcrammap:
	.=128*4+.
	.set	vcram,vcrammap-Sysmap*NPTEPG+SYSTEM
vcrommap:
	.=4+.
	.set	vcrom,vcrommap-Sysmap*NPTEPG+SYSTEM
wbufmap:
	.=WPAGES*4+.
	.set	wbuffer,wbufmap-Sysmap*NPTEPG+SYSTEM

	.=Sysmap+0xC00	| page align
.globl pmapmap
pmapmap:
	.=256*4+.	| allocate P map level 2 map 

|interrupt handlers 
	.text

clkhndl:moveml	#CSAVE,sp@-	|save registers that C clobbers
	jsr	clock
	jra	handler

smdhndl:moveml	#CSAVE,sp@-	|Interphase SMD 2181 disk controller
	jsr	smdintr
	jra	handler

tmhndl:moveml	#CSAVE,sp@-	|CPC Tapemaster half-inch tape controller
	jsr	tmintr
	jra	handler

qtrhndl:moveml	#CSAVE,sp@-	|Quarterback tape drive
	jsr	qtrintr
	jra	handler

mtihndl:moveml	#CSAVE,sp@-	|MTI800
	jsr	mtiintr
	jra	handler

osirrpt:moveml	#CSAVE,sp@-	 |CDC osi receive interrupt
	jsr	osirint
	jra	handler

vchndl:	moveml	#CSAVE,sp@-	|vcmem interrupt
	jsr	vcintr
	jra	handler

ethrhndl:moveml	#CSAVE,sp@-	 |Ethernet interrupt
	jsr	enetintr
	jra	handler

ethr2hndl:moveml #CSAVE,sp@-	 |Ethernet interrupt (for back-back test)
	jsr	net2intr
	jra	handler

ositrpt:moveml	#CSAVE,sp@-	 |CDC osi transmit interrupt
	jsr	osixmit
	jra	handler

ash0r:	moveml	#CSAVE,sp@-	|save registers that C clobbers
	movl	#0,sp@-		|receiver interrupt on port 0
	jra	ashrcv

ash0x:	moveml	#CSAVE,sp@-
	movl	#0,sp@-		|transmit interrupt on port 0
	jra	ashxmt

ash1r:	moveml	#CSAVE,sp@-
	movl	#1,sp@-		|receiver interrupt on port 1
				|fall into receive call
	.globl	ahrint
ashrcv:	jsr	ahrint		|device driver receive int handler
	addql	#4,sp		|pop argument off stack
	jra	handler

ash1x:	moveml	#CSAVE,sp@-
	movl	#1,sp@-		|transmit interrupt on port 1
				|fall thru
	.globl	ahxint
ashxmt:	jsr	ahxint		|handle interrupts from async ports
	addql	#4,sp		|pop argument off stack
	jra	handler		|finish return


|Sdu interrupts
sdu0:	moveml	#CSAVE,sp@-
	movl	#0,sp@-
	jra	sduhndlr

sdu1:	moveml	#CSAVE,sp@-
	movl	#1,sp@-
	jra	sduhndlr

sdu2:	moveml	#CSAVE,sp@-
	movl	#2,sp@-
	jra	sduhndlr

sdu3:	moveml	#CSAVE,sp@-
	movl	#3,sp@-
	jra	sduhndlr

sdu4:	moveml	#CSAVE,sp@-
	movl	#4,sp@-
	jra	sduhndlr

sdu5:	moveml	#CSAVE,sp@-
	movl	#5,sp@-
	jra	sduhndlr

sdu6:	moveml	#CSAVE,sp@-
	movl	#6,sp@-
	jra	sduhndlr

sdu7:	moveml	#CSAVE,sp@-
	movl	#7,sp@-
	jra	sduhndlr

sdu8:	moveml	#CSAVE,sp@-
	movl	#8,sp@-
	jra	sduhndlr

sdu9:	moveml	#CSAVE,sp@-
	movl	#9,sp@-
	jra	sduhndlr

sdu10:	moveml	#CSAVE,sp@-
	movl	#10,sp@-
	jra	sduhndlr

sdu11:	moveml	#CSAVE,sp@-
	movl	#11,sp@-
	jra	sduhndlr

sdu12:	moveml	#CSAVE,sp@-
	movl	#12,sp@-
	jra	sduhndlr

sdu13:	moveml	#CSAVE,sp@-
	movl	#13,sp@-
	jra	sduhndlr

sdu14:	moveml	#CSAVE,sp@-
	movl	#14,sp@-
	jra	sduhndlr

sdu15:	moveml	#CSAVE,sp@-
	movl	#15,sp@-
	jra	sduhndlr

	.globl	sduintr
sduhndlr:	jsr	sduintr		|Handle interrupts from sdu
	addql	#4,sp		|Pop arg, fall into handler
	
handler:moveml	sp@+,#CREST	|restore the registers we saved
	btst	#5,sp@		|did we come from user mode?
	jne	1$		|no, return normally
	tstb	runrun		|else see if we should reschedule
	jne	2$		|yes, simulate a fault entry
1$:	rte

2$:	movw	#256*SHIFT,sp@-	|signal a reschedule fault
	jra	fault

	
|Bus error entry, this has its stack somewhat different.  We will
|call a C routine to save the info then fix the stack to look like a trap.
|These entries will be called directly from interrupt vector.

	.globl	buserr,busaddr
buserr:	moveml	#CSAVE,sp@-	|save registers that C clobbers
	jsr	busaddr		|save the info for a bus or address error
	moveml	sp@+,#CREST	|restore registers
	addql	#8,sp		|pop fcode, aaddr and ireg
	movw	#2*SHIFT,sp@-	|trap type 2
	jra	fault

	.globl	addrerr
addrerr:moveml	#CSAVE,sp@-	|save registers that C clobbers
	jsr	busaddr		|save the info for a bus or address error
	moveml	sp@+,#CREST	|restore registers
	addql	#8,sp		|pop fcode, aaddr and ireg
	movw	#3*SHIFT,sp@-	|trap type 3
	jra	fault

|the actual entry points push a trap number and jmp to trap

illegal:
zerodiv:
chkinst:
trapvinst:
line1010:
line1111:
priviledge:
trap4:
trap5:
trap6:
trap7:
trap8:
trap9:
trap10:
trap11:
trap12:
trap13:
trap14:
trap15:
	movw	#4*SHIFT,sp@-
	jra	fault

trap0:
	movw	#32*SHIFT,sp@-
	jra	fault

trap2:
	movw	#34*SHIFT,sp@-
	jra	fault

trap3:
	movw	#35*SHIFT,sp@-
	jra	fault

trap1:
trace:	movw	#9*SHIFT,sp@-
	jra	fault

| Since we got here from a jsr badint, we want to rearrange the stack
| from ra-sr-pc to sr-pc-ra (ra is the return address saved by the jsr).
| If we do this, everything will look ok to trap (sys/trap.c) and trap
| can identify the bad interrupt from ra, which is saved below the
| normal stuff.  "fault" must remember to remove ra from the stack or
| all hell will break loose when he does his 'rte'.

badint:	movl	sp@,sp@(-4)	|move ra to temp spot above stack
	movw	sp@(4),sp@	|move sr to top of stack
	movl	sp@(6),sp@(2)	|move pc to spot 2 in stack
	movl	sp@(-4),sp@(6)	|move ra to spot 3 in stack
	movw	#24*SHIFT,sp@-
	jra	fault

	.globl	trap
fault:	moveml	#0xFFFF,sp@-	|save all registers except sp
	movl	usp,a0
	movl	a0,sp@(60)	|save usr stack ptr
1$:	jsr	trap		|C handler for traps and faults
	btst	#5,sp@(66)	|check sys bit in original ps, from user mode?
	jne	2$		|no, just continue
	tstb	runrun		|should we reschedule?
	jeq	2$		|no, just return normally
	movw	#256,sp@(64)	|256 is reschedule trap number
	jra	1$		|go back into trap

2$:	movl	sp@(60),a0
	movl	a0,usp		|restore usr stack ptr
	moveml	sp@+,#0x7FFF	|restore all other registers
	cmpw	#24*SHIFT,sp@(4)|did we get here from badint ?
	jne	3$
	movl	sp@(8),sp@(12)	|   yes, remove ra from middle of stack
	movw	sp@(6),sp@(10)	|   and shift rest of stack
	movw	sp@(4),sp@(8)
	movl	sp@,sp@(4)
	addql	#4,sp
3$:	addql	#6,sp		|pop sp and fault number
	rte
|Initialization

	.text
	.globl	start, initmap
start:	moveq	#0,d0		|init all the registers to 0 for consistancy
	movl	d0,d1
	movl	d0,d2
	movl	d0,d3
	movl	d0,d4
	movl	d0,d5
	movl	d0,d6
	movl	d0,d7
	movl	d0,a0
	movl	d0,a1
	movl	d0,a2
	movl	d0,a3
	movl	d0,a4
	movl	d0,a5
	movl	d0,a6

	movl	sp,initmap	|this points to devmap
	jsr	main		|do kernel stuff
	movl	#0x400,sp@-	|starting address in init
	clrw	sp@-		|initial ps, note user mode
	rte			|off to never-never land, 'hope UPBR is ok

|save and restore of register sets to perform context switches
	.data
cid:	.byte	1		|our initial cid

	.text
	.globl	save,presume
save:	movl	sp@+,a1		|return address
	movl	sp@,a0		|ptr to label_t
	moveml	#0xFCFC,a0@	|save d2-d7, a2-a7
	movl	a1,a0@(48)	|save return address
	clrl	d0
	jmp	a1@


|resume is hard because I cannot touch stack while doing it
|for details on how resume flushes the cache, see cflush below

presume:
	movl	sp@(4),d1	|p_addr
	movl	sp@(8),a0	|ptr to label_t
	movl	d1,usrpbr	|set up usr page map
	movw	sr,d0		|save me a copy of status
	movw	#HIGH,sr	|don't bother me until I switch registers
	movl	d1,syspbr	|install map in pbr's
	cmpb	#MAXCID,cid	|have we run out of cid's
	jlt	3$		|no, keep going
	movl	#SYSTEM,a2	|get ready to start touching ram locations
	movw	#1023,d2	|number of data cache entries - 1
	movb	#1,cid		|reset the cache id's
	movb	#1,syscid	|we are going to cache flush to 0 as our cid
1$:	movb	a2@,d1		|just fetch to set this cache cid to 0
	addql	#4,a2		|bump to next cache entry
	dbf	d2,1$		|count while d2 != -1
	movl	#tlb2+16,a2	|pointer to tlb2 registers on cpu board
	movl	#tlb2tags+16,a1	|pointer to tlb2 tags on cpu board
	movw	#512+64-4-1,d2	|number of entries in TLB2 and TLB1
	moveq	#0,d1		|we're going to fill buffers with 0
	andb	#NOTLB,cachectl	|turn off translation buffers
2$:	movl	d1,a1@+		|zero the tags to set cid to 0
	movl	d1,a2@+		|zero the data to set parity properly in tags
	dbf	d2,2$		|finsh the flush
	orb	#DOTLB,cachectl	|now enable translation again
	jra	4$
3$:	addb	#1,cid		|advance to next cid
	movb	cid,syscid	|install new system cid
4$:	movb	cid,usrcid	|install new usr cid
	moveml	a0@+,#0xFCFC	|restore the registers (my registers!)
	movl	a0@,a1
	movw	d0,sr		|spl 0,	supervisor mode
	moveq	#1,d0		|return 1
	jmp	a1@

|Cache Flush

|To do this, it must flush the caches normally just by incrementing the
|current cid.  When we run out of cid's, we must do a real cache flush.
|This is a little tricky.  The object is to make sure that every cache
|entry of both the data and translation caches has either a valid entry
|from the current cid else is set to zero which is not used as a cid
|and hence will be updated when it is referenced.

|First we switch to the new cid which is 1 and validate all data cache
|entries by touching the first 4 pages of the system.  Note that we
|have not switched maps but the entries will still be valid because we
|do not change the low system pages anyway.

	.globl	cflush
cflush:	cmpb	#MAXCID,cid	|have we run out of cid's
	jlt	3$		|no, keep going
	movl	#SYSTEM,a0	|get ready to start touching ram locations
	movw	#1023,d0	|number of data cache entries - 1
	movb	#1,cid		|reset the cache id's
	movb	#1,syscid	|we are going to cache flush to 0 as our cid
1$:	movb	a0@,d1		|just fetch to set this cache cid to 0
	addql	#4,a0		|bump to next cache entry
	dbf	d0,1$		|count while d0 != -1

|Next is the tricky part.  We assume that the first 4 entries have been
|validated by the data cache sweep and that we are running on a page
|corresponding to a validated entry.  We "validate" the remaining entries
|by filling them with zeros which is known to be an unused cid.
|We also know that we won't need to access TLB1 by the time we get around
|to zeroing it.
	movl	#tlb2+16,a0	|pointer to tlb2 registers on cpu board
	movl	#tlb2tags+16,a1	|pointer to tlb2 tags on cpu board
	movw	#512+64-4-1,d0	|number of entries in TLB2 and TLB1
	moveq	#0,d1		|we're going to fill buffers with 0
	movw	sr,sp@-		|save status register
	movw	#HIGH,sr	|an interrupt here gets cache parity errs
	andb	#NOTLB,cachectl	|turn off translation buffers
2$:	movl	d1,a1@+		|zero the tags to set cid to 0
	movl	d1,a0@+		|zero the data to set parity properly in tags
	dbf	d0,2$		|finsh the flush
	orb	#DOTLB,cachectl	|now enable translation again
	movw	sp@+,sr		|let interrupts happen again
	jra	4$
3$:	addb	#1,cid		|advance to next cid
	movb	cid,syscid	|install new system cid
4$:	movb	cid,usrcid	|install new usr cid
	rts


|Validate just make sure that the appropriate level 2 pte entry will
|get reloaded next time it is used.  Note that we do not flush the
|data cache so this had better be a no cache page.

	.globl validate
validate:
	movl	sp@(4),d0	|the virtual address we are trying to validate
	lsrl	#PGSHIFT-2,d0	|convert vaddr to tlb2 index ie (pagenum << 2)
	andl	#L2MASK,d0	|modulo size of tlb2
	movl	#tlb2tags,a0	|get a pointer to tlb2tags
	movl	#tlb2,a1	|and one to tlb2 data
	addl	d0,a0		|adjust pointer to proper entry
	addl	d0,a1		|do it to this pointer too
	movw	sr,d0		|save it
	movw	#HIGH,sr	|don't bug me now
	movl	#0,a0@		|this sets cid to 0
	movl	#0,a1@		|this makes sure its parity is correct
	movw	d0,sr		|resume normal priority
	rts


	.globl	spl7,spl6,spl5,spl1,spl0,splx
|set priority levels

spl7:	movw	sr,d0
	movw	#0x2700,sr
	rts

spl6:	movw	sr,d0
	movw	#0x2600,sr
	rts

spl5:	movw	sr,d0
	movw	#0x2500,sr
	rts

spl1:	movw	sr,d0
	movw	#0x2100,sr
	rts

spl0:	movw	sr,d0
	movw	#0x2000,sr
	rts

splx:	movw	sr,d0		|return current priority
	movw	sp@(6),sr	|the priority he passed as arg
	rts


	.globl idle,idleflg
|Do nothing at priority 0 and come out on next interrupt
	.data
idleflg:.byte	0

	.text
idle:	movb	#1,idleflg	|tell anyone who cares that we were just idle
	stop	#0x2000		|stop at priority 0
	clrb	idleflg		|something must have happened
	rts


	.globl tas
|Test and set a designated location return condition code values

tas:	movl	sp@(8),d1	|delay count in iterations
	movl	sp@(4),a0	|argument is a byte address to tas
1$:	movw	sr,d0		|get priority like an spl
	movl	#HIGH,sr
	tas	a0@		|do it
	jne	2$		|whoops, somebody already has it
	rts

2$:	movw	d0,sr		|let anther interrupts happen for a moment
	jsr	delay		|do nothing for 100 microseconds 
	subql	#1,d1		|decrement
	jge	1$		|ok, let's try again
	movl	#-1,d0		|signal caller we failed
	rts			|give up with -1 error return

	.globl	delay
| Do nothing for a while without touching the bus

delay:	movw	#100,d0
	dbf	d0,.
	rts


|Bad interrupts go thru this table.  Trap can then
|look at the pushed return address and identify the
|offending interrupt.

	.globl	badvec
badvec:
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint
	jsr	badint

	.asciz	"@(#)locore.s	1.14 (Texas Instruments)  83/08/24"
