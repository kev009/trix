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


#include	"../h/param.h"
#include	"../h/calls.h"
#include	"../h/link.h"
#include	"../h/com.h"
#include	"../h/global.h"
#include	"../h/memory.h"
#include	"../h/domain.h"
#include	"../h/thread.h"
#include	"../h/assert.h"
#include 	"../h/devmap.h"
#include 	"../h/vcmem.h"


#define	SDUSLOT		0xFF000000	/* base of SDU card */
#define		MULTIBASE	0x18000		/* address of multibus - nubus map */
#define		SDUIOREG	0x1C000		/* io registers on sdu itself */
#define		CMOSBASE	0x1E000		/* address of cmos ram */
#define		MECBASE		0x30000		/* MB address of 3Com controller */
#define		SMDBASE		0x100000	/* MB address of smd controller */
#define		QTRBASE		0xYYYY
#define CPUSLOT		0xFB000000
#define 	CDATA		0x80000
#define		CTAGS		0x100000
#define		INTRAM		0xE00000	/* offset of interrupt ram */
#define 	CTLSTAT		0xE80000
#define		CPUCONF		0xF00000	/* offset of configuration register */


extern	long	Sysmap[];
extern	long	ashmap[];
extern	long	mecmap[];
extern	long	smdmap[];
extern	long	intrmap[];
extern	long	cmosmap[];
extern	long	multimap[];
extern	long	cdatamap[];
extern	long	ctagsmap[];
extern	long	cpu1map[];
extern	long	cpu2map[];
extern	long	vcconmap[];
extern	long	vcsltmap[];
extern	long	vcrammap[];
extern	long	vcrommap[];
extern	long	tmp1map[];
extern	long	tmp2map[];
extern	long	mnc2map[];
extern	long	mnc1map[];
extern	long	qtrmap[];
extern	long	mec[];

extern	long	usrpbr[];
extern	long	syspbr[];
extern	char	cpuconf[];
extern	char	vccon[];
extern	char	vcram[];
extern	char	vcslt[];
extern	long	tmp1[];
extern	char	tmp2[];

extern int Vid;
int Debug = 0;
extern thread_t threads[];


char	*init_prog;
static	char	init[32] = "/tetc/";


main()
{
	pte_t	*L1;
	pte_t	*ptab;
	int	i;
	int	free;
	domain_t	*dp;
	extern	end, etext;

	L1 = (pte_t *)(SYSTEM - UCHUNK);
	Vid = 1;

	/* map special areas into kernel address space */
	newmap(ashmap, SDUSLOT + SDUIOREG, 1, PG_UW);
	newmap(mecmap, SDUSLOT + MECBASE, 8, PG_UW);
	newmap(smdmap, SDUSLOT + SMDBASE, 1, PG_UW);
	newmap(qtrmap, SDUSLOT + SDUIOREG, 2, PG_UW);
	newmap(cmosmap, SDUSLOT + CMOSBASE, 8, PG_UR);
	newmap(multimap, SDUSLOT + MULTIBASE, 4, PG_UW);
	newmap(cdatamap, CPUSLOT + CDATA, 8, PG_UW);
	newmap(ctagsmap, CPUSLOT + CTAGS, 8, PG_UW);
	newmap(intrmap, CPUSLOT + INTRAM, 1, PG_UW);
	newmap(cpu1map, CPUSLOT + CTLSTAT, 1, PG_UW);
	newmap(cpu2map, CPUSLOT + CPUCONF, 1, PG_UW);
	newmap(mnc2map, devmap.port[0].wakeup & ~PGOFSET, 1, PG_UW);
/*	newmap(mnc1map, devmap.port[0].ioport & ~((long)PGOFSET), 1,PG_UW);*/
	newmap(mnc1map, SDUSLOT, 1,PG_UW);

	/* initmap is set up by the sdu before the 68000 is booted */
	mcopy((char *)initmap, (char *)&devmap, sizeof(devmap));

	/* This is TI stuff to map in vcmem */
	if(devmap.vcmem[0]) {
		/* only one display for now */
		newmap(vcconmap, devmap.vcmem[0], 1, PG_UW);
		newmap(vcsltmap, devmap.vcmem[0]+VC_SLTOFF, 8,PG_UW);
		newmap(vcrammap, devmap.vcmem[0]+VC_RAMOFF, 128, PG_UDAT);
		newmap(vcrommap, devmap.vcmem[0]+VC_ROMOFF, 1, PG_KW);
	}

	strcat(&init[strlen(init)], devmap.console);
	init_prog = init;

	cmapsize = 0;
	for(i = 0 ; i < MAXRAM ; i++)
		cmapsize += devmap.ram[i].ramsize;
	/* don't allow the cmap to overflow the static area */
	if(cmapsize > CMAPSIZE)
		cmapsize = CMAPSIZE;

	printf("\nTRIX 1.0 BOOTED (init=%s)\n cpu type=%d  cpu conf=%x  mem=%dK\n",
		init,
		cpu_type,
		cpuconf[0],
		cmapsize);

	/* set initial stack size depending upon whether page faults work */
	if(cpu_type == 68010)
		stacksize = STACKMIN;
	else
		stacksize = STACKSIZE;

	/* cache system text */
	free = btoc((int)&etext - SYSTEM);
	for(i=0 ; i < free ; i++)
		Sysmap[i] |= PG_C;

	free = btoc((int)&end - SYSTEM);
	for(; i < free ; i++)
		Sysmap[i] |= PG_C;

	/* set video controller to store mode */
	((struct vcmem *)vccon)->vc_freg0 |= VCF_STORE;

	p_init();
	h_init();
	t_init();

	/* lock system/kernel in core */
	free = btoc((int)&end - SYSTEM) + UPAGES;

	for(i = 0 ; i < free ; i++)
		coremap[i] = CP_ALLOC | CP_LOCKED;

	printf("Syspbr=%x Usrpbr=%x\n", syspbr[0], usrpbr[0]);
	printf("First free m=%x, p=%x\n",i,mtop(i));

	/*
	 * Setup the system domain.
	 *
	 * The system domain segment is strange.
	 * We fill in the page table with a user accessable copy of Sysmap.
	 * This pagemap is used for copying in and out and is mapped
	 *   in when the user is running in the SYSTEM domain.
	 * For now we just copy the maps completely,
	 *   we may wish to be more selective in the future
	 */


	if((dp = d_alloc()) == NULLDOMAIN)
		panic("No initial domain\n");
	debug("Initial domain = %x\n", dp);
	dp->d_state |= D_SYS;
	D_SYSTEM = dp;

	/* setup KERNEL segment to point to the already running domain */
	dp->d_tdmap.m_addr = (caddr_t)SYSTEM;
	dp->d_tdmap.m_seg->s_size = ((int)&end - SYSTEM);
	newmap(&dp->d_tdmap.m_seg->s_L1map[0], pg_alloc(0), 1, PG_UW);
	newmap(tmp1map, dp->d_tdmap.m_seg->s_L1map[0], 1, PG_KW);
	validate(tmp1);
	for(i=0 ; i < 256 ; i++)
		if(Sysmap[i])
			tmp1[i] = (Sysmap[i] & (~0xF0)) | PG_V | PG_UW;

	/* initialize all devices */
	clk_init();
	intinit();

	/* spawn a system thread */
	T_CURRENT = NULLTHREAD;
	if((T_CURRENT = t_spawn(NULLTHREAD, &D_SYSTEM->d_subdom, system, 0, 0)) ==
           NULLTHREAD)
		panic("can't spawn initial thread");
	T_CURRENT = schedule();
	/* this is here for assembler use in spl emulation */
	{	extern domain_t	*D_CURRENT;
		D_CURRENT = T_CURRENT->t_subdom->d_domain;
	}

	memmap(T_CURRENT);
}


/*
 * This is magic from TI stuff 
 */

newmap(map, paddr, npages, flags)
long	*map;
long	paddr;
int	npages;
long	flags;
{
	long proto;
	proto = paddr | flags | PG_V;
	while(npages--) {
		*map++ = proto;
		proto += 1024;
	}
}


mpanic()
{
	printf("Help: returned from initial resched\n");
	for(;;);
}


/* return physical address corresponding to a system virtual addr */

long sysaddr(p)
caddr_t p;
{
	return((Sysmap[btop(p - (caddr_t)SYSTEM)] & ~PGOFSET) |
			((unsigned)p & PGOFSET));
}


/*
 *  return interrupt address corresponding to named interrupt
 *  the named interrupt is actually an address in low	memory
 *  each interrupt is 4 bytes in low memory and each corresponding word in
 *    the cpu interrupt ram is also 4 bytes.
 */

long intaddr(p)
int *p;
{
	return(devmap.cpu[0] |  (INTRAM + ((long)p & 0x3FFL)));
}


/* print info for bad interrupt */

printint(d0, d1, d3, a0, a1, stat, rpc)
short stat;
{
	printf(" bad interrupt: ps = %x, rpc = %x\n", stat, rpc);
}


splpanic()
{
	panic("spl trap called from user domain\n");
}
