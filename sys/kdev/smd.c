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


/*
 *    Driver for Fujitsu M2312K or M2351A disk on an Interphase SMD 2181
 *    controller.
 *
 *    Multiple logical disks are implemented, but multiple physical disks
 *    per controller are not.  All device monitoring hooks (dk_ stuff) are
 *    in place.  (iostat stuff is gathered by bio routines).
 *
 *    Does not currently support scattered reads/writes.
 */

#include "../h/param.h"
#include "../h/args.h"
#include "../h/link.h"
#include "../h/calls.h"
#include "../h/com.h"
#include "../h/memory.h"
#include "../h/unixtypes.h"
#include "../h/global.h"
#include	"param.h"
#include	"fsys.h"
#include	"inode.h"
#include	"cache.h"
#include	"smd.h"
#include	"confram.h"


/* cache element of active transfer */
struct	cache	*smdactive;
struct  list	smdque;
int	smd_debug = 0;


#define RSWAIT    /*6*/100            /* time to wait after RESET & INIT */
#define SMDWLIM	  20           /* timeout on interrupt */


/*
 *  Macros to convert between bytes, sectors, and disk blocks.
 */
#define sctob(x)     ((x) << btsshft)
#define btosc(x)     ((x) >> btsshft)
#define dtosc(x)     ((x) << stdshft)
#define sctod(x)     ((x) >> stdshft)

static short open_flag = 0;   /* has drive been opened? */


/*
 *  Externals.
 */
extern char smdvec[];         /* interrupt vector, given to sdu */
extern char cmos[];           /* map in configuration ram */


/*
 *  disk parameters needed by smd_start().
 */
static unsigned int secsize;
static unsigned int secpercyl;
static unsigned int secperhead;
static unsigned int secofset;

/*
 *  shifting factors to go between bytes, sectors, and disks
 */
static unsigned short btsshft;       /* log2 of bytes per sector */
static unsigned short stdshft;       /* log2 of sectors per disk block */

/*
 *  Disk controller control blocks.
 */
static	struct iopb  *smdiopb;	/* io parameter block we always use */
static	struct uib   *smduib;	/* unit initialization block */


/*
 *  Control registers for 2181 controller (in MultiBus io space).
 */

struct smdregs {         /* cmd, xmb, msb, and lsb go into the high-order */
    char cmd;            /* byte of 68k word, which goes to the low-order */
    char junk1[3];       /* byte of NuBus word, which maps to a single    */
    char xmb;            /* byte of MultiBus io space.                    */
    char junk2[3];
    char msb;
    char junk3[3];
    char lsb;
};

static struct smdregs *smdregs;



/*
 * smd_init - Gets disk parameters from cmos ram then resets and
 *	       initializes the controller.  Note that MultiBus to
 *	       NuBus maps are allocated in cache.c	
 */

smd_init()
{
    extern long smd[];
    extern char  uncached[];
    extern int   uncachmap[];
    struct cmos_ram cmos_ram;
    register short i, j;
    int	 temp;
    char *p;

    if(open_flag)   /* only enter on first open or lost interrupt */
        return;

    /* get a physical block for the disk controller */
    newmap(&uncachmap[CACHES], pg_alloc(), 1, PG_UW);
    smdiopb = (struct iopb *)&uncached[CACHES*1024];
    smduib = (struct uib *)&uncached[CACHES*1024 + sizeof(struct iopb)];
    /* Read Unit Initialization Block out of the configuration ram */
    smdcmos(&cmos_ram, sizeof(cmos_ram));
    for(i = 0; i < MCARDS; i++) {
        if (strcmp(cmos_ram.m[i].m_name, DISKNAME) == 0) {
            for(j = 0, p = (char *)smduib; j < sizeof(*smduib); j++, p++)
                *p = cmos_ram.m[i].m_data[j];
            break;
        }
    }
	if(i >= MCARDS)
		panic("disk not found in configuration ram");

	/* Set disk parameters needed by smdstart() */
	secsize = smduib->bpshi << 8;
	secsize |= smduib->bpslow;
	secofset = secsize - 1;
	secperhead = smduib->spertrk;
	secpercyl = smduib->ntracks * secperhead;

	/*
	 *  Calculate shifting factors.  (Assumes that blocks are an integral
	 *  number of sectors long.)  btsshft is log2(secsize), stdshft is
	 *  log2(BLKSIZE/secsize).
	 */
	for(btsshft = 0, i = secsize; !(i & 0x01); btsshft++, i >>= 1);
	for(stdshft = 0, i = BLKSIZE/secsize; !(i & 0x01); stdshft++, i >>= 1);

	/* Fill in constant fields in the IO Parameter Block */
	smdiopb->dmacnt  = NBURST;
	smdiopb->options = (ABSADDR | BUSWIOPB | ABSBUF | BUSWBUF);
	smdiopb->seg = 0;
	smdiopb->next = 0;
	smdiopb->unit = 0;     /* only 1 unit for now */
	smdiopb->ioa = SMD;

	sdumap(0x80000, sysaddr(smdiopb), 2);
	temp = 0x80000 | (sysaddr(smdiopb) & PGOFSET);

	/* 
	 *  Get a pointer to the controller registers and point the
	 *  controller to the iopb.
	 */
	smdregs = (struct smdregs *)&smd[SMD];
	smdregs->xmb = (temp >> 16);
	smdregs->msb = (temp >> 8);
	smdregs->lsb = temp;

	/* establish an interrupt handler with the sdu */
/*    sdugram(smdvec, IO_INTR, DISKINT);*/

	/* RESET the controller */
	smdiopb->comm = RESET;
	smdiopb->error = smdiopb->stat = 0;
	smdregs->cmd = (CLEAR_GO | BUSW);

	/* wait for RESET to finish */
	do {
		smddelay(RSWAIT);
		debug("waiting for state: %d\n",  smdiopb->stat);
	} while(smdiopb->stat == 0 || smdiopb->stat == CMDBUSY);

	if(smdiopb->stat != CMDOK) {
		panic("DISK RESET ERROR\n");
		return;
	}

	/*
	 *  Somewhat of a kludge here.  If the controller is accessed too soon
	 *  after the RESET, a NuBus timeout occurrs.
	 */
	smddelay(RSWAIT);

	/* INITIALIZE the controller */
	smdiopb->comm = INITIALIZE;
	smdiopb->error = smdiopb->stat = 0;
	sdumap(0x80800, sysaddr(smduib), 2);
	smdiopb->bfraddr = 0x80800 | (sysaddr(smduib) & PGOFSET);
	smdregs->cmd = (CLEAR_GO | BUSW);

	/* wait for INITIALIZE to finish */
	do {
		smddelay(RSWAIT);
		debug("waiting for state: %d\n", smdiopb->stat);
	} while(smdiopb->stat == CMDBUSY);

	if(smdiopb->stat != CMDOK) {
                panic("DISK INITIALIZE ERROR\n");
                return;
	}

	l_list(&smdque);

	if(!open_flag)
		open_flag++;
}


/* copy every 4th byte from the area mapped in by cmos[] to p[] */

smdcmos(p, count)
char *p;
int count;
{
	register char *cmp;

	for(cmp = &cmos[0] ; count-- ; cmp += 4, p++)
		*p = *cmp;
}


/* sort  the element to the unit's queue, start io if disk is idle */

smd_queue(cp)
register struct cache	*cp;
{
	int s;

	/* right now, just add element to the end of the list */

	if(smd_debug)
		printf("s_q(%x,%d,%c)  ", cp,cp->c_bno,
    			(cp->c_state & C_RDBSY) ? 'R' : 'W');

	if(cp->c_state & C_USBSY)
		printf("queueing usbsy block\n");

	s = splx(SPL_DISK);			/* protect queue */
	l_link(&smdque, &cp->c_next);
	splx(s);

	smd_start();
}


/* start a disk transaction on the given cache element */

smd_start()
{
	register struct	cache	*cp;
	register unsigned short	hn, sn, cn;	/* head, sector, cylinder # */
	struct link *l;
	int s;

	/*
	 *  Verify that there is indeed a request and then mark the
	 *  disk as active.
	 */

	s = splx(SPL_DISK);
	if(smdactive != NULLCACHE) {
		splx(s);
		return;
	}
	if((l = l_first(&smdque)) != NULLLINK) {
		l_unlink(l);
		cp = LINKREF(l, c_next, struct cache *);
	}
	else
		cp = NULLCACHE;

	if((cp == NULLCACHE) || (!(cp->c_state & C_IOBSY))) { 
		splx(s);
		return;
	}

	smdactive = cp;

	splx(s);

	if(smd_debug)
		printf("s_s(%x,%d,%x)  ", cp, cp->c_bno, cp->c_state);

	smdiopb->scnt = btosc(cp->c_length);
	smdiopb->bfraddr = cp->c_adr;

	/* Calculate cylinder, sector, and head and put them into the smdiopb */
	cn = dtosc(cp->c_bno) / secpercyl;
	sn = dtosc(cp->c_bno) % secpercyl;     /* sector within cylinder */
	hn = sn / secperhead;
	sn %= secperhead;

	/* Set other goodies in smdiopb */
	smdiopb->cyl  = cn;
	smdiopb->sect = sn;
	smdiopb->head = hn;
	smdiopb->stat = smdiopb->error = 0;
	smdiopb->comm = (cp->c_state & C_RDBSY) ? READ_CMD : WRITE_CMD;

	/* start the xfer by writing to RO. */
	smdregs->cmd = (CLEAR_GO | BUSW);     /* kick the disk (finally) */
}


/*
 *  disk interrupt handler
 *  check status, handle temp fields on a transfer which has been broken
 *    into smaller transfers
 *  do an smdstart() to start the next disk action.
 */

smd_int()
{
	register struct	cache	*cp = smdactive;

	/* Ignore a spurious interrupt. */
	if(cp == NULLCACHE) {
		printf("Spurious disk interrupt\n");
	    	return;
	}

	/*
	 *  Get pointer to the interrupting cache entry, mark disk as inactive,
	 *  and update device monitoring gunk.
	 *
	 *  Check controller status and act accordingly.  (Remember that
	 *  the 2181 automatically attempts error recovery.)
	 */

	switch(smdiopb->stat) {
	    case CMDERR:
		/* Unrecoverable error. */
		printf("Error=%d\n", smdiopb->error);
		panic("Unrecoverable disk error\n");
		cachedone(cp, (l_first(&smdque) == NULLLINK));
		break;

	    case CMDOK:
		/* Completed I/O */
		smdactive = NULLCACHE;
		/* complete I/O on cache entry and starts new transaction */
		cachedone(cp, (l_first(&smdque) == NULLLINK));
		break;

	    case 0:		/* command not started yet */
	    case CMDBUSY:	/* command still in progress */
		/*
		 *  Ignore unsolicited interrupt.  Either our interrupt will
		 *  come along or disk_watch will restart us.
		 */
		printf("Strange disk interrupt\n");
		break;

            default:
		/* 
		 *  Shouldn't happen.  If it does, the mb->nb map pointing
		 *  to the iopb may have been clobbered.
		 */
		printf("smdiopb->stat = 0x%x\n", smdiopb->stat);
		panic("smdiopb->stat invalid");
	}

	smd_start();			/* keep the disk busy */
}


/* delay for ms milliseconds */

smddelay(ms)
{
	ms = ms*10 + 1;
	while(--ms > 0)
		delay();
}
