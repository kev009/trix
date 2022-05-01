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
#include	"../h/args.h"
#include	"../h/calls.h"
#include	"../h/devmap.h"
#include	"../h/memory.h"
#include	"../h/link.h"
#include	"../h/status.h"
#include	"../h/protocol.h"
#include	"../h/unixtypes.h"
#include	"../h/assert.h"
#include	"cache.h"


#define	min(x,y)	((x) <= (y) ? (x) : (y))


/* functions for manipulating the cache */
static	struct	cache	*askcache();
static	struct	cache	*getcache();
static			putcache();
static			flscache();
static	struct	cache	cache[CACHES];

/* "clock" for fifo algorithm */
static	int	tim;


#define	ANYDISK	-1

int	smd_init(), smd_start(), smd_queue();

struct	disk	{
	off_t	size;		/* size of virtual disk */
	off_t	offset;		/* offset of virtual disk on drive */
	/* hardware routines to deal with controlling disk drive */
	int	(*init)();	/* initialization routine for disk */
	int	(*start)();	/* start routine for disk */
	int	(*queue)();	/* interrupt handler for disk */
}  disks[] = {
	/* the size and offset fields get initialized from devmap */
  { 0x7C1800,	0x1F800,   smd_init, smd_start, smd_queue },	/* A: the root */
  { 0x1054000,	0x7FF800,  smd_init, smd_start, smd_queue },	/* B: swap area */
  { 0x483A800,	0,         smd_init, smd_start, smd_queue },	/* C: whole disk */
  { 0x7C1800,	0x185D000, smd_init, smd_start, smd_queue },	/* D */
  { 0x1B50000,	0x203D000, smd_init, smd_start, smd_queue },	/* E */
  { 0xCAC800,	0x3B8E000, smd_init, smd_start, smd_queue },	/* F */
  { 0x2FBE000,	0x185D000, smd_init, smd_start, smd_queue }	/* G: usr filesys */
};

#define	DISKS	(sizeof(disks)/sizeof(struct disk))


hndl_t
disk_make(disk)
unsigned disk;
{
	static	firsttime = 1;
	extern	disk_handler();

	if(disk >= DISKS)
		panic("disk_make on nonexistant disk\n", disk);

	if(firsttime) {
		extern	char	uncached[];
		extern	int	uncachmap[];
		int	c;

		firsttime = 0;

		/* initialize the cache */
		/* the SDU window for the cache is at 0x90000 */
		/* 0x80000 - 0x807FF is map entries for smd control block */
		/* 0x80800 - 0x80FFF is scratch map entries */

		/* allocate enough physical memory for the block cache */
		for(c = 0 ; c < CACHES*BLKSIZE ; c += 1024)
			newmap(&uncachmap[c/1024], pg_alloc(), 1, PG_UW);

		/* initialize the cache */
		for(c = 0 ; c < CACHES ; c++) {
			/* to say that this setup is not generalized is ...*/
			cache[c].c_state = 0;
			/* invalidate entry */
			cache[c].c_bno = -1;
			cache[c].c_disk = ANYDISK;
			cache[c].c_tim = -c;
			/* standard entries are 1 BLKSIZE long */
			cache[c].c_length = BLKSIZE;
			/* initialize data pointer */
			cache[c].c_data = &uncached[c*BLKSIZE];
			/* compute address for dma */
			cache[c].c_adr = 0x90000 + (c << BLKSHFT);
			/* use a map entry */
			sdumap(cache[c].c_adr, sysaddr(cache[c].c_data),
				BLKSIZE/1024);
		}
	}

	/* initialize the disk structure */
/*
	disks[disk].offset = devmap.disk[disk].offset;
	disks[disk].size = devmap.disk[disk].size;
*/
	debug("disk: %d   offset=%x  size=%x\n",
		disk, disks[disk].offset, disks[disk].size);
		
	(*disks[disk].init)();
	return(p_kalloc(disk_handler, disk, 0));
}


int	disk_print();
int	(*f4_function)() = disk_print;

disk_print()
{
	struct	cache	*cp;

	/* flush the cache on the given disk */
	for(cp = &cache[0] ; cp < &cache[CACHES] ; cp++)
		printf("%d: %d %d %x\n",
			cp->c_bno, cp->c_tim, cp->c_hits, cp->c_state);
}


static	long
cache_gstat(disk, type, dflt)
int	type;
long	dflt;
{
	extern	long	trixtime;

	switch(type) {
	    case STATUS_ATIME:
	    case STATUS_CTIME:
	    case STATUS_MTIME:
		return(trixtime);

	    case STATUS_IDENT:
		return((long)(&disks[disk]));

	    case STATUS_PROT:
		return(PROT_BLOCK);

	    case STATUS_ACCESS:
		return(0622);	

	    case STATUS_OWNER:
		return(0);

	    case STATUS_SUSES:
		return(0);

	    case STATUS_ISIZE:
		return(disks[disk].size);
	}

	return(dflt);
}




static	long
cache_pstat(d, type, val)
register union	direct	*d;
int	 type;
long	 val;
{
	/* ignore PUTSTATs */
	return(cache_gstat(d, type, -1));
}


disk_handler(disk, mp)
register struct message	*mp;
{
	debug("disk_handle: %x\n", mp->m_opcode);
	if(disk >= DISKS)
		panic("attempt to access nonexistant disk\n");

	switch(mp->m_opcode) {
	    case GETSTAT:
	    case PUTSTAT:
		mp->m_ercode = simplestat(mp, cache_gstat, cache_pstat, disk);
		return(0);

	    case CONNECT:
		mp->m_ercode = E_ASIS;
		return(0);

	    case CLOSE:
panic(" Close\n");
		/* flush the cache */
		if(flscache(disk))
			return(1);
		/* must invalidate cache entries that point to this disk */
		break;

	    case READ:
	    {	register int	o;
		register int	i;
		register struct	cache	*cp;

		debug("read(%d) %d %d\n",disk,mp->m_param[0],mp->m_dwsize);
		if(!krerun()) {
			/* check against size of logical disk */
			if(mp->m_param[0] < 0 ||
			   mp->m_param[0] + mp->m_dwsize > disks[disk].size) {
				if(mp->m_param[0] >= disks[disk].size) {
					mp->m_ercode = E_MAXSIZE;
					return(0);
				}
				else {
					mp->m_dwsize =
						disks[disk].size - mp->m_param[0];
				}
			}

			/* add offset of logical disk to offset */
			mp->m_param[0] += disks[disk].offset;
			/* the count of transferred blocks are in param[2] */
			mp->m_param[2] = 0;
		}

		o = mp->m_param[0] & BLKMASK;
		while(mp->m_param[2] < mp->m_dwsize) {
			i = min(BLKSIZE-o, mp->m_dwsize-mp->m_param[2]);
			cp = getcache(disk,
				      (mp->m_param[0] + mp->m_param[2]) / BLKSIZE,
				      C_READ);
			if(cp == NULLCACHE) {
				return(1);
			}
			if(mp->m_param[2] + i < mp->m_dwsize) {
				/* start preread before store */
				askcache(disk,
					 ((mp->m_param[0]+mp->m_param[2])/BLKSIZE)+1,
					 C_READ);
			}
			mp->m_param[2] +=
			    kstore(mp->m_dwaddr+mp->m_param[2], &cp->c_data[o], i);
debug("after kstore param[2] = %d\n", mp->m_param[2]);
			putcache(cp, 0);
			o = 0;
		}

		if(mp->m_param[1] != -1) {
			/* read ahead specified - ask cache to get it */
			askcache(disk,
				 (mp->m_param[1]+disks[disk].offset)/BLKSIZE,
				 C_READ);
		}

		mp->m_param[0] = mp->m_param[2];
		break;
	    }

	    case WRITE:
	    {	register int	o;
		register int	i;
		register struct	cache	*cp;

		debug("WRITE(%d) %d %d\n",disk,mp->m_param[0],mp->m_dwsize);
		if(!krerun()) {
			/* check against size of logical disk */
			if(mp->m_param[0] < 0 ||
			   mp->m_param[0] + mp->m_dwsize > disks[disk].size) {
				if(mp->m_param[0] >= disks[disk].size) {
					mp->m_ercode = E_MAXSIZE;
					return(0);
				}
				else {
					mp->m_dwsize =
						disks[disk].size - mp->m_param[0];
				}
			}

			/* add offset of logical disk to offset */
			mp->m_param[0] += disks[disk].offset;
			/* the count of transferred blocks are in param[2] */
			mp->m_param[2] = 0;
		}

		o = mp->m_param[0] & BLKMASK;
		while(mp->m_param[2] < mp->m_dwsize) {
			i = min(BLKSIZE-o, mp->m_dwsize-mp->m_param[2]);
			cp = getcache(disk,
				      (mp->m_param[0] + mp->m_param[2]) / BLKSIZE,
				      i==BLKSIZE?0:C_READ);
			if(cp == NULLCACHE) {
				return(1);
			}
			mp->m_param[2] +=
			    kfetch(mp->m_dwaddr + mp->m_param[2], &cp->c_data[o], i);
			putcache(cp, C_WRITE);
			o = 0;
		}

		mp->m_param[0] = mp->m_param[2];
		break;
	    }

	    case UPDATE:
		if(flscache(disk)) {
			mp->m_ercode = E_OK;
			return(1);
		}
		break;

	    default:
		mp->m_ercode = E_OPCODE;
		return(0);
	}

	mp->m_ercode = E_OK;
	return(0);
}


static	struct	cache  *
askcache(disk, bn, rw)
register int	disk, bn;
{
	register struct	cache	*cp, *scp;

	/* askcache is used for prereading data into the cache */

	for(scp = NULLCACHE, cp = &cache[0] ; cp < &cache[CACHES] ; cp++) {
		if(cp->c_disk == disk && cp->c_bno == bn) {
			/* its already in the cache */
			/* if we were using length this would be harder */
			if((cp->c_state & (C_USBSY | C_IOBSY)) == 0) {
				/* update the time for fifo flushing */
				cp->c_tim = tim++;
			}
			return(cp);
		}

		if(cp->c_state == 0) {
			/* this entry is usable */
			if(scp == NULLCACHE || scp->c_tim > cp->c_tim)
				scp = cp;
		}
	}

	if(scp == NULLCACHE) {
		/* no free slot to preread into */
		return(NULLCACHE);
	}

	/* reassign the cache entry */
	scp->c_disk = disk;
	scp->c_bno = bn;
	scp->c_tim = tim++;
	scp->c_hits = 0;
	if(rw == C_READ) {
		/* start up IO to preread in block */
		scp->c_state = C_RDBSY;
		(*disks[scp->c_disk].queue)(scp);
	}
	return(scp);
}


static	struct	cache  *
getcache(disk, bn, rw)
register int	disk, bn;
{
	register struct	cache  *cp;

	/* try to force the block into the cache */

	if((cp = askcache(disk, bn, rw)) == NULLCACHE) {
		/* no place for it in the cache - start output */
		flscache(ANYDISK);
		return(NULLCACHE);
	}

	if(cp->c_state & C_USBSY) {
		/* its in use - go to sleep and try again */
		cp->c_state |= C_UWAIT;
		ksleep(0, cp);
		return(NULLCACHE);
	}

	/* there is a problem here with two threads beating on the same cp */
	if(cp->c_state & C_IOBSY) {
		/* IO is in progress - go to sleep and try again */
		cp->c_state |= C_IWAIT;
		ksleep(0, cp);
		return(NULLCACHE);
	}

	/* lock it for the caller */
	cp->c_state |= C_USBSY;
	cp->c_hits++;
	return(cp);
}


static
putcache(cp, rw)
register struct	cache  *cp;
{
	if(!(cp->c_state & C_USBSY))
		panic("putcache called with nonbusy entry\n");

	if(rw == C_WRITE)
		cp->c_state |= C_DIRTY;

	if(cp->c_state & C_UWAIT) {
		cp->c_state &= ~C_UWAIT;
		kwakeup(0, cp);
	}

	cp->c_state &= ~C_USBSY;
}


static
flscache(disk)
{
	register struct	cache  *cp, *scp;

	/* start output on oldest dirty cache entry */
	for(scp = NULLCACHE, cp = &cache[0] ; cp < &cache[CACHES] ; cp++) {
		if((disk == ANYDISK || cp->c_disk == disk) &&
		   cp->c_state == C_DIRTY) {
			/* entry is dirty but not in the midst of IO */
			if(scp == NULLCACHE || scp->c_tim > cp->c_tim)
				scp = cp;
		}
	}

	if(scp != NULLCACHE) {
		/* for now we poll the disk waiting for this I/O to complete */
		ASSERT(scp->c_state == C_DIRTY);
		/* put the thread to sleep */
		ksleep(0, scp);

		/* start the I/O transaction */
		scp->c_state |= C_WRBSY | C_IWAIT;
		(*disks[scp->c_disk].queue)(scp);
		return(1);
	}

	/* there is nothing left to flush */
	return(0);
}


/* mark cache entry as having completed IO and start a new transaction */

cachedone(cp, idle)
register struct cache	*cp;
{
	register struct cache	*scp;
	int	 disk = cp->c_disk;

	/* should take argument describing status (error condition) if any */
	cp->c_state &= ~(C_IOBSY | C_DIRTY);
	if(cp->c_state & C_IWAIT) {
		cp->c_state &= ~C_IWAIT;
		kwakeup(0, cp);
	}

	if(!idle)
		return;

	/* start a flush I/O transaction on the disk */
	for(scp = NULLCACHE, cp = &cache[0] ; cp < &cache[CACHES] ; cp++) {
		if(cp->c_disk == disk && cp->c_state == C_DIRTY) {
			/* entry is dirty but not in the midst of IO */
			if(scp == NULLCACHE || scp->c_tim > cp->c_tim)
				scp = cp;
		}
	}
	if(scp != NULLCACHE) {
		scp->c_state |= C_WRBSY;
		(*disks[scp->c_disk].queue)(scp);
	}
}



/* THESE ROUTINES ARE HERE FOR QTR.C */

/* wait for pending IO to complete (restarting if necessary) */

IOWait(cp)
register struct cache  *cp;
{
	int	s;

	/* for now we poll the disk waiting for this I/O to complete */
	s = splx(SPL_WAKEUP);
	while(cp->c_state & C_IOBSY) {
		cp->c_state |= C_IWAIT;
		I_sleep(0, cp);
	}
	splx(s);
}


/* mark cache entry as having completed IO and start a new transaction */

IODone(cp, idle)
register struct cache	*cp;
{
	register struct cache	*scp;
	int	 disk = cp->c_disk;

	/* should take argument describing status (error condition) if any */
	cp->c_state &= ~(C_IOBSY | C_DIRTY);
	if(cp->c_state & C_IWAIT) {
		cp->c_state &= ~C_IWAIT;
		I_wakeup(0, cp);
	}

	if(!idle)
		return;

	/* start a flush I/O transaction on the disk */
	for(scp = NULLCACHE, cp = &cache[0] ; cp < &cache[CACHES] ; cp++) {
		if(cp->c_disk == disk && cp->c_state == C_DIRTY) {
			/* entry is dirty but not in the midst of IO */
			if(scp == NULLCACHE || scp->c_tim > cp->c_tim)
				scp = cp;
		}
	}
	if(scp != NULLCACHE) {
		scp->c_state |= C_WRBSY;
		(*disks[scp->c_disk].queue)(scp);
	}
}
