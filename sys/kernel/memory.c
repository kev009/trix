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
#include	"../h/devmap.h"


extern long syspbr[];
extern long usrpbr[];
extern long tmp1map[];
extern long tmp2map[];
extern long tmp1[];
extern long tmp2[];
extern long tmp3map[];
extern long tmp4map[];
extern char tmp3[];
extern char tmp4[];
extern long tmprmap[];	/* this tmp page can only be read uncached */
extern char tmpr[];

segment_t segments[SEGMENTS];


/*
 * a segment is a mappable region of memory (or disk) that represents a
 *  window on a handle
 * semantically it has the property that the data is read sometime before
 *  a memory access is allowed to occur and is written sometime after it has
 *  been modified
 * segments with a null handle simply represent zero initialized memory
 * for now there is no sharing of segments
 * an eventual goal is to share segments with compatible windows on the same
 *  handle
 */


s_print(s)
segment_t  *s;
{
	debug("S(%x):  size=0x%x  mode=0x%x\n", s, s->s_size, s->s_mode);
}


/* allocate a segment of the specified size */

segment_t *
s_alloc(size, mode)
off_t	 size;
{
	register segment_t *s;
	register int	i;

	/* verify that size is on proper boundaries */
	ASSERT(ctob(btoc(size)) == size);
	ASSERT(size < MAXMEM);
	debug("s_alloc(%x)\n",size);

	for(s = &segments[0] ; s < &segments[SEGMENTS] ; s++) {
		if(s->s_mode != 0)
			continue;
		s->s_mode = S_ALLOC | mode;
		s->s_size = 0;
		for(i = 0 ; i < SSIZE ; i++)
			s->s_L1map[i] = 0;
		mapexpand(btoc(size), s, PG_UDAT);
		s->s_size = size;
		return(s);
	}
	panic("out of segments");
	return(NULLSEGMENT);
}


/* expand a segment */

s_expand(s, incr)
register segment_t *s;
register off_t	  incr;
{
	register off_t	nincr;

	debug("s_expand(%x, %x)\n", s, incr);

	if(incr < 0)
		/* segments cannot shrink */
		return(0);
	nincr = btoc(incr);		/* align to page boundary */

	/* allocate new memory area */

	mapexpand(nincr, s, PG_UDAT);
	s->s_size += ctob(nincr);
	return(0);
}


/* free the segment and any memory/disk it uses */

s_free(s)
segment_t *s;
{
	if(T_MAPPED == s)
		T_MAPPED = NULLSEGMENT;
	if(W_MAPPED == s)
		W_MAPPED = NULLSEGMENT;
	if(D_MAPPED == s)
		D_MAPPED = NULLSEGMENT;
	mapfree(s);
	/* mark segment as free */
	s->s_mode = 0;
}


/* test whether a window is within given map */

max_within(m, start, size)
register struct	map	*m;
off_t	 start;
off_t	 size;
{
	register struct	segment	*s = m->m_seg;

	start -= (off_t)m->m_addr;
	if(s->s_mode & S_STACK)
		return(start >= SLENG - s->s_size && start + size <= SLENG);
	else
		return(start >= 0 && (start + size) <= s->s_size);
}


/* read c bytes from segment s (offset o) and write beginning and t */

s_read(c, t, s, o)
register int        c;
caddr_t    t;
segment_t  *s;
off_t      o;
{
	char	**vptab;
	register char  *vf;
	register int   n;

	if(s->s_mode & S_STACK) {
		ASSERT(o >= (SLENG - s->s_size));
		ASSERT((o + c) <= SLENG);
	}
	else {
		ASSERT(o >= 0);
		ASSERT((o + c) <= s->s_size);
	}

	NEWMAP(tmp1map, s->s_L1map[btol1(o)], PG_UW);
	uvalidate(tmp1);
	vptab = (char **)(tmp1) + btol2(o);
	vf = (char *)tmpr + (o & PAGEMASK);

	while(c > 0) {
		/* map in current page */
		if(((int)vptab & PAGEMASK) == 0) {
			/* at the end of a L1 page go to the next one */
			NEWMAP(tmp1map, s->s_L1map[btol1(o)], PG_UW);
			uvalidate(tmp1);
			vptab = (char **)(tmp1);
		}
		/* read around the cache to avoid stale data */
		NEWMAP(tmprmap, *vptab++, PG_RTMP);
		uvalidate(tmpr);
		resume();

		if((n = (PAGESIZE - (o & PAGEMASK)) ) > c)
			n = c;

		mcopy(vf, t, n);
		vf = (char *)tmpr;
		t += n;
		c -= n;
		o += n;
	}
resume();
}


/* write c bytes to segment s (offset o) and  from t */

s_write(c, t, s, o)
register int        c;
caddr_t    t;
segment_t  *s;
off_t      o;
{
	char	**vptab;
	register char  *vf;
	register int   n;

	if(s->s_mode & S_STACK) {
		ASSERT(o >= (SLENG - s->s_size));
		ASSERT((o + c) <= SLENG);
	}
	else {
		ASSERT(o >= 0);
		ASSERT((o + c) <= s->s_size);
	}

	NEWMAP(tmp1map, s->s_L1map[btol1(o)], PG_UW);
	uvalidate(tmp1);
	vptab = (char **)(tmp1) + btol2(o);
	vf = (char *)tmp2 + (o & PAGEMASK);

	while(c > 0) {
		/* map in current page */
		if(((int)vptab & PAGEMASK) == 0) {
			/* at the end of a L1 page go to the next one */
			NEWMAP(tmp1map, s->s_L1map[btol1(o)], PG_UW);
			uvalidate(tmp1);
			vptab = (char **)(tmp1);
		}
		/* write through the cache to update stale data */
		NEWMAP(tmp2map, *vptab++, PG_WTMP);
		uvalidate(tmp2);

		if((n = (PAGESIZE - (o & PAGEMASK)) ) > c) n = c;

		mcopy(t, vf, n);
		vf = (char *)tmp2;
		t += n;
		c -= n;
		o += n;
	}
	/*
	 *  this is here because the cache seems to not invalidate entries
	 *    that don't match virtual addresses
	 */
	resume();
}


s_copy(c, sf, of, st, ot)
segment_t  *sf, *st;
off_t      of, ot;
{
	register long  cf;
	register long  ct;
	register long  n;
	char **ftab;
	char **ttab;

	if(c == 0) return;
	panic("scopy called\n");
	if(sf == 0) panic("attempt to copy into null segment\n");
	if((of + c) > sf->s_size)
		panic("s_copy: read size too big\n");
	if((ot + c) > st->s_size)
		panic("s_copy: write size too big\n");

	/* map in page maps */
	NEWMAP(tmp1map, sf->s_L1map[btol1(of)], PG_UW);
	uvalidate(tmp1);
	NEWMAP(tmp2map, st->s_L1map[btol1(ot)], PG_UW);
	uvalidate(tmp2);

	ftab = (char **)(tmp1) + btol2(of);
	ttab = (char **)(tmp2) + btol2(ot);

	while(c > 0) {
		/* map in pages */
		/* read around the cache to avoid stale data */
		NEWMAP(tmprmap, *ftab, PG_RTMP);
		uvalidate(tmpr);
		/* write through the cache to update stale data */
		NEWMAP(tmp4map, *ttab, PG_WTMP);
		uvalidate(tmp4);
		cf = (long)of & PAGEMASK;
		ct = (long)ot & PAGEMASK;
		if(cf == ct) {
			if((n = PAGESIZE - ct) > c)
				n = c;
			mcopy((char *)tmpr+cf, (char *)tmp4+ct, n);
			of +=n;
			ot +=n;
			ftab++;
			ttab++;
			if(((int)ftab & PAGEMASK) == 0) {
			    /* at the end of a L1 page go to next one */
			    NEWMAP(tmp1map, sf->s_L1map[btol1(of)], PG_UW);
			    uvalidate(tmp1);
			    ftab = (char **)(tmp1);
			}
			if(((int)ttab & PAGEMASK) == 0) {
			    /* at the end of a L1 page go to next one */
			    NEWMAP(tmp2map, st->s_L1map[btol1(ot)], PG_UW);
			    uvalidate(tmp2);
			    ttab = (char **)(tmp2);
			}
		}			
		if(cf < ct) {
			/* size of block is limited by pt boundary */
			if((n = PAGESIZE - ct) > c)
				n = c;
			mcopy((char *)tmpr+cf, (char *)tmp4+ct, n);
			of += n;
			ot += n;
			ttab++;
			if(((int)ttab & PAGEMASK) == 0) {
			    /* at the end of a L1 page go to next one */
			    NEWMAP(tmp2map, st->s_L1map[btol1(ot)], PG_UW);
			    uvalidate(tmp2);
			    ttab = (char **)(tmp2);
			}
		}
		if(cf > ct) {
			/* size of block is limited by pf boundary */
			if((n = PAGESIZE - cf) > c)
				n = c;
			mcopy((char *)tmpr+cf, (char *)tmp4+ct, n);
			of += n;
			ot += n;
			ftab++;
			if(((int)ftab & PAGEMASK) == 0) {
			    /* at the end of a L1 page go to next one */
			    NEWMAP(tmp1map, sf->s_L1map[btol1(of)], PG_UW);
			    uvalidate(tmp1);
			    ftab = (char **)(tmp1);
			}
		}
		c -= n;
	}
}


/* extend the segment by incr pages */

mapexpand(incr, seg, flags)
register off_t	incr;
register segment_t  *seg;
long	 flags;
{
	register pte_t	*L1;
	register long	i;

	if(seg->s_mode & S_STACK) {
		L1 = &seg->s_L1map[btol1(SLENG - seg->s_size)];
		i = 0x100 - btol2(seg->s_size);
		if(i != 0x100) {
			NEWMAP(tmp1map, *L1, PG_UW);
			uvalidate(tmp1);
		}
		while(--incr >= 0) {
			if((i & 0xFF) == 0) {
				/* need L1 entry, get a page table */
				NEWMAP(--L1, (long *)pg_alloc(0), PG_UW);
				NEWMAP(tmp1map, *L1, PG_UW);
				uvalidate(tmp1);
				i = 0x100;
			}
			NEWMAP(&tmp1[--i], pg_alloc(1), flags);
		}
	}
	else {
		L1 = &seg->s_L1map[btol1(seg->s_size)];
		i = btol2(seg->s_size);
		if(i != 0) {
			NEWMAP(tmp1map, *L1++, PG_UW);
			uvalidate(tmp1);
		}
		while(--incr >= 0) {
			if((i & 0xFF) == 0) {
				/* need L1 entry, get a page table */
				NEWMAP(L1, (long *)pg_alloc(0), PG_UW);
				NEWMAP(tmp1map, *L1++, PG_UW);
				uvalidate(tmp1);
				i = 0;
			}
			NEWMAP(&tmp1[i++], pg_alloc(0), flags);
		}
	}
}


/* free the segment's memory maps */

mapfree(seg)
register segment_t  *seg;
{
	register pte_t	*L1, *L1end;
	register int	i;

	if(seg->s_mode & S_STACK) {
		/* L1 map of first allocated page */
		L1end = &seg->s_L1map[btol1(SLENG-PAGESIZE)];
		/* L1 map of next page to be allocated */
		L1 = &seg->s_L1map[btol1((SLENG-PAGESIZE) - seg->s_size)];

		/* free all the full L1 pages */
		while(L1 < L1end) {
			ASSERT(*L1end != 0);
			NEWMAP(tmp1map, *L1end, PG_UW);
			uvalidate(tmp1);
			for(i = 0 ; i <= 0xFF ; i++) {
				ASSERT(tmp1[i] != 0);
				pg_free(tmp1[i]);
			}
			pg_free(*L1end--);
		}

		/* free the last partial L1 page */
		if((i = 0x100 - btol2(seg->s_size)) != 0x100) {
			NEWMAP(tmp1map, *L1end, PG_UW);
			uvalidate(tmp1);
			while(i < 0x100) {
				ASSERT(tmp1[i] != 0);
				pg_free(tmp1[i++]);
			}
			pg_free(*L1end);
		}
	}
	else {
		/* L1 map of first allocated page */
		L1 = &seg->s_L1map[0];
		/* L1 map of next page to be allocated */
		L1end = &seg->s_L1map[btol1(seg->s_size)];

		/* free all the full L1 pages */
		while(L1 < L1end) {
			ASSERT(*L1 != 0);
			NEWMAP(tmp1map, *L1, PG_UW);
			uvalidate(tmp1);
			for(i = 0 ; i <= 0xFF ; i++) {
				ASSERT(tmp1[i] != 0);
				pg_free(tmp1[i]);
			}
			pg_free(*L1++);
		}

		/* free the last partial L1 page */
		if((i = btol2(seg->s_size)) != 0) {
			NEWMAP(tmp1map, *L1, PG_UW);
			uvalidate(tmp1);
			while(--i >= 0) {
				ASSERT(tmp1[i] != 0);
				pg_free(tmp1[i]);
			}
			pg_free(*L1);
		}
	}
}


/*
 *  this is the TRIX version of a context switch
 *  when we exit the kernel execution will continue in the new domain and thread
 *  at the moment:
 *	the thread stack is not remapped and protected
 *	the data window is not protected
 */

memmap(t)
register thread_t  *t;
{
	static mapfirst = 1;
	register pte_t	*L1;
	register pte_t	*ptab;
	register int	i;
	register domain_t  *d = t->t_subdom->d_domain;

	if(SYSDOMAIN(d) && mapfirst) {
		/* map new domain - kernel is always mapped */
		debug("mapping system domain: ");

		L1 = (pte_t *)(SYSTEM - ctob(UPAGES)) + btol1(SYSTEM);
		ptab = (pte_t *)(d->d_tdmap.m_seg->s_L1map);
		/* pmap contains pte (system domain < 256K) */
		*L1 = *ptab;
		mapfirst = 0;
		flushcache = 1;
	}

	/* map new domain if necessary (SYSDOMAIN is always mapped) */
	if(!SYSDOMAIN(d) && d->d_tdmap.m_seg != D_MAPPED) {
		ASSERT(d->d_tdmap.m_addr == USERBASE);

		L1 = (pte_t *)(SYSTEM - ctob(UPAGES)) + btol1(d->d_tdmap.m_addr);
		ptab = (pte_t *)(d->d_tdmap.m_seg->s_L1map);
		for(i = 0 ; i < SSIZE ; i++)
			/* pmap contains pte's */
			*L1++ = *ptab++;
		D_MAPPED = d->d_tdmap.m_seg;
		flushcache = 1;
	}

	/* map new data window if necessary (there may not be a data window) */
	if(t->t_dwmap.m_seg != W_MAPPED) {
	    if(t->t_dwmap.m_seg != NULLSEGMENT) {
		ASSERT(t->t_dwmap.m_addr == DWBASE);

		L1 = (pte_t *)(SYSTEM - ctob(UPAGES)) + btol1(t->t_dwmap.m_addr);
		ptab = (pte_t *)(t->t_dwmap.m_seg->s_L1map);
		for(i = 0 ; i < SSIZE ; i++)
			/* pmap contains pte's */
			*L1++ = *ptab++;
	    }

	    W_MAPPED = t->t_dwmap.m_seg;
	    flushcache = 1;
	}

	/* map thread stack if neccessary */
	if(t->t_smap.m_seg != T_MAPPED) {
		ASSERT(t->t_smap.m_addr == STACKBASE);

		L1 = (pte_t *)(SYSTEM - ctob(UPAGES)) + btol1(t->t_smap.m_addr);
		ptab = (pte_t *)(t->t_smap.m_seg->s_L1map);
		for(i = 0 ; i < SSIZE ; i++)
			/* pmap contains pte's */
			*L1++ = *ptab++;
		T_MAPPED = t->t_smap.m_seg;
		flushcache = 1;
	}

	/* flush out the cache if any of the maps got changed */
	if(flushcache) {
		resume();
		flushcache = 0;
	}
}


/*
 * Allocate a free page from core and return a pointer. At the moment
 * a caddr_t is returned enventually, a core/disk pointer will be
 */

pg_alloc(fill)
{
	static	 int	next = 0;
	register int	i, j;
	register int	paddr;

	for(i = next ; i != (next-1) ; ) {
		if(coremap[i] == CP_FREE) {
			paddr = mtop(i);
			coremap[i] |= CP_ALLOC;
			if(fill == 0) {
				NEWMAP(tmp2map, paddr, PG_WTMP);
				uvalidate(tmp2);
				mfill(tmp2, PAGESIZE, 0);
			}
			next = i;
			return(paddr);
		}
		if(++i >= cmapsize)
			i = 0;
	}

	panic("can't allocate page");
	return(0);
}


pg_free(paddr)
caddr_t paddr;
{
	int pnum;

	pnum = ptom(paddr);
	coremap[pnum] = CP_FREE;
}


/*
 *  mtop -	Convert map index to ram physical address.  This routine
 *		knows about the location of ram boards and converts the
 *		coremap index to a NuBus address.
 */

mtop(mind)
register mind;
{
	register struct ramhunk *ram;
	register n;

	n = mind;  /* save original for panic message */
	for (ram = devmap.ram ; ram < &devmap.ram[MAXRAM] ; ram++) {
		if (n < ram->ramsize)
			return((ram->ramaddr + ctob(n)));
		n -= ram->ramsize;
	}

	panic("mtop called with too big index 0x%X\n",mind);
	return(0L);
}


long
ptom(paddr)
{
	register struct ramhunk *ram;
	register int count = 0;

	for(ram = devmap.ram ; ram < &devmap.ram[MAXRAM] ; ram++) {
 		if((paddr >= ram->ramaddr) && 
		   (paddr < (ram->ramaddr + ptob(ram->ramsize))))
			return(count + btop(paddr - ram->ramaddr));
		count += ram->ramsize;
	}

	panic("ptom called with bad ram address %x\n",paddr);
	return(0);
}
