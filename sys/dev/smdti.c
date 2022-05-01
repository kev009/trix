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


/* Driver for Interphase SMD 2181 controller -- from TI source (cjt 4/84) */

#include "param.h"
#include "systm.h"
#include "buf.h"
#include "conf.h"
#include "dir.h"
#include "user.h"
#include "iomsg.h"
#include "socket.h"
#include "map.h"
#include "pte.h"
#include "ioctl.h"
#include "86include/interrupt.h"
#include "86include/confram.h"

typedef unsigned char BYTE;
typedef unsigned short WORD;

#define UNIT(x) ((minor(x) >> 4) & 0xF)
#define LOGDRIVE(x) (minor(x) & 0xF)

/* controller registers */
#define SMD	0x40	/* base IO address - hardware dependent */
#define RCMD	(SMD+0)	/* command register (only writable register) */
#define RSTATUS	(SMD+0)	/* status register */
#define RXMB	(SMD+1)	/* bits 24-16 of the iopb address */
#define RMSB	(SMD+2)	/* bits  15-8 of the iopb address */
#define RLSB	(SMD+3)	/* bits   7-0 of the iopb address */

/* possible values to write to the RCMD register - may be 'or'ed */
#define GO		1
#define CLEARINT	2
#define CLEAR_GO	(GO|CLEARINT)
#define BUSW		0x20	/* 0 for byte mode, 1 for word mode */

/* status bits for RSTATUS */
#define BUSY	0x01
#define INTPEND	0x02		/* operation has been completed */
#define U1RDY	0x10
#define U2RDY	0x20
#define U3RDY	0x40
#define U4RDY	0x80

struct iopb {
	BYTE	comm;		/* command */
	BYTE	options;	/* command options */
	BYTE	stat;		/* set to 0 before doing each cmd */
	BYTE	error;		/* error code/number retries */
	BYTE	unit;		/* unit select (0-3) */
	BYTE	head;           /* head select */
	WORD	cyl;		/* cyl no. */
	WORD	sect;   	/* sector no. */
	WORD	scnt;   	/* no. sectors to r/w */
	unsigned dmacnt:8;	/* busburst lngth (1-256 - 16 common)*/
	unsigned bfraddr:24;	/* memory address of buffer */
	WORD	ioa;		/* io address (set equal to SMD) */
	WORD	seg;		/* memory seg rel. addr */
	unsigned i_reserved:8;	/* (unused byte) */
	unsigned next:24;	/* linked iopb address */
};

/* commands for comm field in iopb */
#define READ	0x81
#define WRITE	0x82
#define VERIFY	0x83
#define TRKFORMAT 0x84
#define MAP	0x85
#define	INITIALIZE 0x87
#define RESTORE	0x89
#define SEEK	0x8A
#define ZEROSEC	0x8B
#define RESET	0x8F

/* command options */
#define LINKIOPB 0x80
#define ABSADDR	0	/* (choose ABSADDR or RELADDR) */
#define RELADDR	0x20
#define BUSWIOPB 0x10	/* 0 for byte mode, 1 for word mode */
#define RSRV	0x8	/* if set, dual port drive is RESERVED after cmd done*/
#define	ABSBUF	0	/* (choose ABSBUF or RELBUF) */
#define	RELBUF	0x2
#define BUSWBUF	0x1	/* 0 for byte mode, 1 for word mode */


/* status codes for the stat field in iopb (returned by controller) */
#define CMDOK	0x80	/* command successul, ready for next command */
#define CMDBUSY	0x81	/* command in progress, busy */
#define CMDERR	0x82	/* command completed with an error */

/* error codes (if stat field in iopb set to CMDERR) */
/* (not used yet) */
#define EC	0x80	/* set if error correction has been applied */
#define BD	0x40	/* set if a restore and reseek sequence was done */
#define RS	0x10	/* set if bad data was moved into memory (see uib) */
#define RTRYMASK 0xF	/* mask to use to get number of rotational retries */

/* for dmacnt field in iopb */
#define NBURST	16

#define MBMAPSIZE (0x8000)     /* mb->nb map for 32k */

#define RSWAIT    6            /* time to wait after RESET & INIT */
#define RSPRI     20           /* no signals during sleep */
#define SMDWLIM	  20           /* if an expected interrupt doesn't come   */
                               /* in SMDWLIM seconds, smdwatch will reset */
                               /* the controller.                         */

/* Bit numbers to turn on in dk_busy for device monitoring. */
#define DK0     0
#define SWP	1

/* Externals. */
extern char smdvec[];         /* interrupt vector, given to sdu */
extern char cmos[];           /* map in configuration ram */

/* Special disk buffer headers. */
struct buf smdtab;            /* ptr to a list of smd buffers */
struct buf rsmdbuf;           /* buffer hdr used for raw i/o  */

/* disk parameters needed by smdstart(). */
static short open_flag = 0;   /* has drive been opened? */

/* Disk controller control blocks. */
static struct iopb smdiopb;          /* io parameter block we always use */

#define MAXUNITS 4		/* max number of disk drives */
#define MAXDISK  8		/* max number of partitions/drive */

struct uib {
	BYTE	ntracks;	/* number of heads/unit */
	BYTE	spertrk;	/* sectors/track */
        BYTE    bpslow;         /* bytes/sector (low byte) */
        BYTE    bpshi;          /* bytes/sector (high byte) */
	BYTE	gap1;   	/* bytes in gap 1 */
	BYTE	gap2;		/* bytes in gap 2 */
	BYTE	interlv;	/* interleave factor */
	BYTE	retries;	/* retry count */
	BYTE	ecc_enab;	/* error correction enable (0 or 1) */
	BYTE	rsk_enab;	/* reseek enable (0 or 1) */
	BYTE	mbd_enab;	/* move bad data enable (0 or 1) */
	BYTE	ibh_enab;	/* increment by head enable (0 or 1) */
	BYTE	dualport;	/* dual port drive (0 or 1) */
	BYTE	int_change;	/*interrupt on status change (0 or 1)*/
	BYTE	spiral;		/* spiral skew factor */
	BYTE	u_res1;		/* reserved byte 1 */
	BYTE	u_res2;		/* reserved byte 2 */
};

struct dinfo {			/* info block for each drive type */
  struct uib smduib;
  unsigned int secsize;
  unsigned int secpercyl;
  unsigned int secperhead;
  unsigned short btsshft;       /* log2 of bytes per sector */
  unsigned short stdshft;       /* log2 of sectors per disk block */
};

struct dinfo M2312K = {		/* 80MB Fujitsu microdisk */
  { 7, 18, 0, 4, 20, 22, 2, 3, 1, 1, 0, 1, 0, 0, 9, 0, 0},
  1024, 18*7, 18, 10, 0
};

struct dinfo M2351A = {		/* 400MB Fujitsu eagle */
  { 20, 25, 0, 4, 28, 31, 3, 3, 1, 1, 0, 1, 0, 0, 12, 0, 0},
  1024, 20*25, 25, 10, 0
};

struct dinfo CDC9766 = {	/* 300MB CDC storage module drive */
  { 19, 18, 0, 4, 20, 22, 2, 3, 1, 1, 0, 1, 0, 0, 11, 0, 0},
  1024, 19*18, 18, 10, 0
};

struct linfo {			/* logical structure of a physical drive */
  long size;			/* length of partition in bytes */
  long offset;
};

struct linfo microformat[MAXDISK] = {
  0x7C1800,	0x1F800,	/* A: the root */
  0x1054000,	0x7FF800,	/* B: swap area */
  0x483A800,	0,		/* C: the whole disk */
  0x7C1800,	0x185D000,	/* D */
  0x1B50000,	0x203D000,	/* E */
  0xCAC800,	0x3B8E000,	/* F */
  0x2FBE000,	0x185D000	/* G: whole disk minus root & swap area */
};

struct linfo cdcformat[MAXDISK] = {
  281466000,	0		/* A: the whole disk */
};

struct linfo eagleformat[MAXDISK] = {
  842*20*25*1024,	0	/* A: the whole disk */
};

struct DParams {		/* info on drive configuration */
  short open;			/* <>0 if unit is initialized */
  struct dinfo *drive_info;	/* info block for this drive */
  struct linfo *log_info;	/* logical disks on this drive */
} dparams[MAXUNITS] = {
  0, &M2312K, microformat,
  0, &M2351A, eagleformat,
  0, &CDC9766, cdcformat,
  0, 0
};

/* Junk for the lost disk interrupt guardian. */
int smdwatch();               /* watches for lost disk interrupts */
static short smdwstart = 0;   /* has smdwatch been started ? */
static int smdwticks = 0;     /* # of ticks since last smd interrupt */
static int track_to_format = -1;  /* which track to format, -1 means none */
static int format_state = 0;	/* indicate where we are in formatting */

/* Junk for MultiBus -> NuBus maps for controller's dma. */
static long smdbase;          /* base of MultiBus -> NuBus map entries */
extern long sdumalloc();      /* allocates a MultiBus -> NuBus map */

/* Control registers for 2181 controller (in MultiBus io space). */
struct smdregs         /* cmd, xmb, msb, and lsb go into the high-order   */
{                      /* byte of a 68k word, which goes to the low-order */
    char cmd;          /* byte of a NuBus word, which maps to a single    */
    char junk1[3];     /* byte of MultiBus io space.                      */
    char xmb;
    char junk2[3];
    char msb;
    char junk3[3];
    char lsb;
};

static struct smdregs *smd;

/*-----------------------------------------------------------------*/
/* smdopen - initializes the controller.  Allocates MultiBus to    */
/*           NuBus maps when disk is openede for the first time.   */
/*-----------------------------------------------------------------*/

smdopen(dev,flag)
  dev_t dev;
  {	int iopb_offset;     /* offset of iopb in area mapped by smdbase */
	int smdwake();
	int temp,unit;
	register short i, j;
	register struct DParams *d;

	if ((unit = UNIT(dev)) > MAXUNITS) {
	  u.u_error = EIO;
	  return;
	}
	d = &dparams[unit];	/* pointer to info for this unit */

	if (open_flag == 0) {	/* only enter on first open */

	  /* Allocate MultiBus->NuBus map entries to point to buffer, iopb,
	   * and a one entry pad.
	   */
	  smdbase = (int)sdumalloc(0L, MBMAPSIZE+NBPG+ctob(2));
	  ASSERT(smdbase != 0L);
	  iopb_offset = MBMAPSIZE+NBPG;

	  /* Fill in constant fields in the IO Parameter Block. */
	  smdiopb.dmacnt  = NBURST;
	  smdiopb.options = (ABSADDR | BUSWIOPB | ABSBUF | BUSWBUF);
	  smdiopb.seg = 0;
	  smdiopb.next = 0;
	  smdiopb.ioa = SMD;

	  /* Fill a MultiBus -> NuBus map entry to point to the iopb. */
	  sdumap(smdbase+iopb_offset, sysaddr(&smdiopb), 2);
	  temp = (smdbase + iopb_offset) | (sysaddr(&smdiopb) & PGOFSET);

	  /* Get a pointer to the controller registers and point the
	   *  controller to the iopb.
	   */
	  smd = (struct smdregs *)&mio[SMD];
	  smd->xmb = (temp >> 16);
	  smd->msb = (temp >> 8);
	  smd->lsb = temp;

  	  /* Establish an interrupt handler with the sdu. */
	  sdugram(smdvec, IO_INTR, DISKINT);

	  /* RESET the controller. */
	  smdiopb.comm = RESET;
	  smdiopb.error = smdiopb.stat = 0;
	  smd->cmd = (CLEAR_GO | BUSW);

	  /* wait for RESET to finish */
	  timeout(smdwake, &smdiopb, RSWAIT);
	  sleep(&smdiopb, RSPRI);
	  if (smdiopb.stat != CMDOK) {
	    printf("DISK RESET ERROR\n");
	    u.u_error = EIO;       /* seems to be the closest error code */
	    return;
	  }

	  /* Somewhat of a kludge here.  If the controller is accessed too
	   * soon after the RESET, a NuBus timeout occurs.
	   */
	  timeout(smdwake, &smdiopb, RSWAIT);
	  sleep(&smdiopb, RSPRI);

	  open_flag++;
	}

	/* now do the per drive stuff */
	if (!unit_init(unit)) {
	  printf("DISK INITIALIZE ERROR\n");
          u.u_error = EIO;
	  return;
	}

	/* start lost interrupt watcher */
	if (!smdwstart) {
	  timeout(smdwatch, &smdwstart, HZ);
	  smdwstart++;
	}
}


/* send parameters about physical drive to disk controller */
unit_init(unit)
  {	register struct DParams *d;

	d = &dparams[unit];

	if (d->open == 0) {
	  /* INITIALIZE the controller with params for this drive */
	  smdiopb.comm = INITIALIZE;
	  smdiopb.unit = unit;
	  smdiopb.error = smdiopb.stat = 0;
   
   	  /* point iopb to uib */
 	  sdumap(smdbase, sysaddr(&d->drive_info->smduib), 2);
	  smdiopb.bfraddr = smdbase | (sysaddr(&d->drive_info->smduib) & PGOFSET);
	  smd->cmd = (CLEAR_GO | BUSW);

	  /* wait for INITIALIZE to finish */
	  timeout(smdwake, &smdiopb, RSWAIT);
	  sleep(&smdiopb, RSPRI);
	  if (smdiopb.stat != CMDOK) return(0);

	  d->open = 1;
	}

	return(1);
}

/*  smdwake - issues a wakeup on chan.  */
smdwake(chan)
 long chan;
 {	wakeup(chan);
}

/*--------------------------------------------------------------------*/
/* smdstrategy - Checks for boundary errors, sets up temp fields in   */
/*               case transfer must be broken up, queues the request, */
/*               and makes sure that the disk is running.             */
/*--------------------------------------------------------------------*/

smdstrategy(bp)
  register struct buf *bp;
  {	int unit, logdrive, nblks, s;
	register struct DParams *d;
	register struct linfo *l;

	/* find out drive number, make sure it's initialized */
	unit = UNIT(bp->b_dev);
	d = &dparams[unit];
	if (d->open==0 && unit_init(unit)==0) {
	  bp->b_flags |= B_ERROR;    
	  iodone(bp);
	  return;
	}

	/* check for track format request.  If this is one, we just have
	 * to stick the buffer onto the request queue.
	 */
	if (bp==&rsmdbuf && track_to_format!=-1)
	  bp->b_xblkno = 0;	/* something for smdsort to work with */
	else {
	  /* Grab logical device number and size of xfer (in blocks). */
	  logdrive = LOGDRIVE(bp->b_dev);
	  nblks = (bp->b_bcount+BSIZE-1) >> BSHIFT;

	  /* Check for bad unit or block number. */
	  if (unit>=MAXUNITS || logdrive>=MAXDISK || bp->b_blkno<0) {
	    bp->b_flags |= B_ERROR;    
	    iodone(bp);
	    return;
	  }
	  l = &d->log_info[logdrive];

	  /* Return eof if beyond end of disk. */
	  if ((s = dtob(bp->b_blkno)) >= l->size) {
	    bp->b_resid = bp->b_bcount;
	    iodone(bp);
	    return;
	  }

	  /* Catch request which extends beyond end of disk. */
	  if ((s + bp->b_bcount) > l->size) bp->b_bcount = l->size - s;

	  /* Initialize the temorary block number, byte count, 
	   *  and buffer address fields.
	   */
	  bp->b_xblkno = bp->b_blkno + btod(l->offset);
	  bp->b_resid = bp->b_xcount = bp->b_bcount;
	  bp->b_xaddr = bp->b_un.b_addr;
	}

	/* Set the disk priority, sort the request queue (by xblkno), make
	 * sure that the disk is active, and restore the priority level.
	 */
	bp->av_forw = (struct buf *)NULL;
	s = spl5();
	smdsort(&smdtab,bp);

	if (smdtab.b_active == 0) smdstart();
	splx(s);
}


/*-------------------------------------------------------------*/
/*  smdstart - Figure out sectors, cylinders, etc and initiate */
/*             a disk activity after filling in the smdiopb.   */
/*-------------------------------------------------------------*/

smdstart()
  {	register struct buf *bp;
	register struct dinfo *d;
	register long temp;
	register short hn, sn, cn;          /* head, sector, cylinder # */
	long nuaddr;                        /* addr on NuBus of buffer  */
	long save_count, bytes_left, nbytes;
	int offs, xclick;
	caddr_t save_addr;
	int dkn;

	/* Verify that there is indeed a request and then mark the
	 *  disk as active.
	 */
	if ((bp = smdtab.b_actf) == NULL) return;
	smdtab.b_active++;

	/* check for track format request and handle it specially */
	if (bp==&rsmdbuf && track_to_format!=-1) {
	  smdiopb.unit = UNIT(bp->b_dev);
	  d = dparams[smdiopb.unit].drive_info;
	  smdiopb.bfraddr = 0;
	  temp = d->smduib.ntracks;
	  smdiopb.scnt = temp;
	  smdiopb.cyl = track_to_format / temp;
	  smdiopb.head = track_to_format % temp;
	  smdiopb.sect = 0;
	  smdiopb.stat = smdiopb.error = 0;
	  smdiopb.comm = (format_state==0) ? TRKFORMAT : VERIFY;
	  smd->cmd = (CLEAR_GO | BUSW);     /* kick the disk (finally) */
	  return;
	}

	/*Take care of transfers which won't fit thru the MultiBus -> NuBus
	map in one shot or which span ram cards.  We will go thru this loop
	once for each ram card which we will be mapping in.  The basic idea
	is to find out how many bytes are left on this card (left in b_xaddr 
	by baddr()), map them in, go to the next card, and so on until all 
	the entire transfer is mapped in.  Note that we only map in an "extra
	page" (to take care of xfers which aren't on page boundaries) the
	first time thru the loop.*/

	bp->b_xcount = save_count = min(bp->b_resid, MBMAPSIZE);
	save_addr  = bp->b_xaddr;
	offs = (int)bp->b_xaddr & PGOFSET;
	bytes_left = save_count;

	xclick = 1;
	while (bytes_left > 0) {
	  nuaddr = baddr(bp);
	  nbytes = save_count - bytes_left;
	  sdumap((int)(smdbase+offs+nbytes), nuaddr,
		 (int)btoc(bp->b_xcount)+xclick);
	  bp->b_xaddr += bp->b_xcount;
	  bytes_left  -= bp->b_xcount;
 	  bp->b_xcount = bytes_left;
	  xclick = 0;
	}

	smdiopb.unit = UNIT(bp->b_dev);
	d = dparams[smdiopb.unit].drive_info;

	bp->b_xaddr = save_addr;
	bp->b_xcount = save_count;
	smdiopb.scnt = (save_count + d->secsize - 1) >> d->btsshft;
	smdiopb.bfraddr = smdbase | ((int)save_addr & PGOFSET);

	/* Calculate cylinder, sector, head and put them into the smdiopb. */
	temp = bp->b_xblkno << d->stdshft;
	cn = temp / d->secpercyl;
	sn = temp % d->secpercyl;   /* sector within cylinder */
	hn = sn / d->secperhead;
	sn %= d->secperhead;

	/* Set other goodies in smdiopb. */
	smdiopb.cyl  = cn;
	smdiopb.sect = sn;
	smdiopb.head = hn;
	smdiopb.stat = smdiopb.error = 0;
	smdiopb.comm = (bp->b_flags&B_READ) ? READ : WRITE;

	/* Start the xfer by writing to RO. */
	smd->cmd = (CLEAR_GO | BUSW);     /* kick the disk (finally) */

	/*  Update all of the various device monitoring gunk. */
	dkn = (bp->b_dev == swapdev) ? SWP : DK0;
	dk_busy |= 1 << dkn;	/* this disk is busy */
	dk_numb[dkn] += 1;	/* # of transfers this disk */
	/* # of 64 byte blks xferred */
	dk_wds[dkn] += save_count >> 6;
}


/*--------------------------------------------------------------*/
/*  smdintr - entered when the disk finishes its task and sends */
/*            an interrupt to the cpu.  Checks status, handles  */
/*            temp fields on a transfer which has been broken   */
/*            into smaller transfers, and does an smdstart() to */
/*            start the next disk action.                       */
/*--------------------------------------------------------------*/

struct { short code; char *msg; } dskmsg[] = {
  0x10,	"disk not ready",
  0x11, "invalid disk address",
  0x12, "seek error",
  0x13, "ecc code error - data field",
  0x14, "invalid command code",
  0x16, "invalid section in command",
  0x18, "bus timeout",
  0x1a, "disk write protected",
  0x1b, "unit not selected",
  0x1c, "no address mark - header field",
  0x1e, "drive faulted",
  0x23, "uncorrectable error",
  0x26, "no sector pulse",
  0x27, "data overrun",
  0x28, "no index pulse on write format",
  0x29, "sector not found",
  0x2a, "id field error - wrong head",
  0x2b, "invalid sync in data field",
  0x2d, "seek timeout error",
  0x2e, "busy timeout",
  0x2f, "not on cylinder",
  0x30, "rtz timeout",
  0x31, "format overrun on data",
  0x40, "unit not initialized",
  0x42, "gap specification error",
  0x4b, "seek error",
  0x4c, "mapped header error",
  0x51, "bytes/sector specification error",
  0x52, "interleave specification error",
  0x53, "invalid head address",
  0,	0
};

smdintr()
  {	register struct buf *bp;
	register struct dinfo *d;
	register int i;

	/* Ignore a spurious interrupt. */
	smdwticks = 0;                   /* reset lost interrupt counter */
	if (!smdtab.b_active) return;

	/* Get pointer to the interrupting buffer, mark disk as inactive,
	 *  and update device monitoring gunk.
	 */
	bp = smdtab.b_actf;
	smdtab.b_active = 0;
	dk_busy &= ~(1 << ((bp->b_dev == swapdev) ? SWP : DK0));

	/* Check controller status and act accordingly.  (Remember that
	 *  the 2181 automatically attempts error recovery.)
	 */
	switch (smdiopb.stat) {
	  case CMDERR:
		/* Unrecoverable error. */
		deverror(bp,smdiopb.error,0);
		for (i = 0; dskmsg[i].code != 0; i += 1)
		  if (dskmsg[i].code == smdiopb.error) {
		    printf("disk error: %s\n",dskmsg[i].msg);
		    break;
		  }
		printf("cyl=%d, head=%d, sect=%d",
			smdiopb.cyl,smdiopb.head,smdiopb.sect);
		if (bp==&rsmdbuf && track_to_format!=-1)
		  printf(" (while %s track %d)",
			 (format_state==0) ? "formatting" : "verifying",
			 track_to_format);
		printf("\n");
		bp->b_flags |= B_ERROR;
		break;

	  case CMDOK:
		/*  Update temporary counters.  If there is more to do for
		 *  this request, start disk again.
		 */
		if (bp==&rsmdbuf && track_to_format!=-1) {
		  if (format_state != 0) break;
		  format_state = 1;	/* verify after format operation */
		  smdstart();
		  return;
		}

		d = dparams[UNIT(bp->b_dev)].drive_info;
		bp->b_xaddr  += smdiopb.scnt << d->btsshft;
		bp->b_xblkno += smdiopb.scnt << d->stdshft;
		bp->b_resid  -= smdiopb.scnt << d->btsshft;

		if (bp->b_resid > 0) {	/* more to do ? */
		  smdstart();		/* yup, keep going */
		  return;		/* on this buffer. */
		}
		break;

	  case CMDBUSY:
		/* Ignore unsolicited interrupt.  Either our interrupt will
		 *  come along or smdwatch will restart us.
		 */
		printf("received disk interrupt with busy controller\n");
		return;

	  default:
		/* Shouldn't happen.  If it does, the mb->nb map pointing
		 *  to the iopb may have been clobbered.
		 */
		printf("smdiopb.stat = 0x%x\n", smdiopb.stat);
		panic("smdiopb.stat invalid");
	}

	/*  Move next buffer to front of queue and release current buffer.
	 *  Start the next disk activity.
	 */
	smdtab.b_actf = bp->av_forw;
	bp->b_resid = 0;
	iodone(bp);
	smdstart();
}


/*-----------------------------------------------------------------*/
/* smdread - raw read from device.                                 */
/*-----------------------------------------------------------------*/

smdread(dev)
  dev_t dev;
  {	register struct dinfo *d = dparams[UNIT(dev)].drive_info;

	if (u.u_count & (d->secsize - 1)) {
	  printf("SMD DRIVER: u.u_count = 0x%x\n", u.u_count);
	  printf("RAW READS SHOULD BE MULTIPLES OF %d BYTES\n", d->secsize);
	}
	physio(smdstrategy, &rsmdbuf, dev, B_READ);
}



/*-----------------------------------------------------------------*/
/* smdwrite - raw write to device.                                 */
/*-----------------------------------------------------------------*/
 
smdwrite(dev)
  dev_t dev;
  {	register struct dinfo *d = dparams[UNIT(dev)].drive_info;

	if (u.u_count & (d->secsize - 1)) {
	  printf("SMD DRIVER: u.u_count = 0x%x\n", u.u_count);
	  printf("RAW WRITES SHOULD BE MULTIPLES OF %d BYTES\n", d->secsize);
	}
	physio(smdstrategy, &rsmdbuf, dev, B_WRITE);
}



/*-----------------------------------------------------------------*/
/*  smdwatch - Wake up every second and if an interrupt is pending */
/*             but nothing has happened increment a counter.  If   */
/*             the counter reaches SMDWLIM, assume the interrupt   */
/*             has been lost and restart the request.              */
/*-----------------------------------------------------------------*/

smdwatch(junk)
  caddr_t junk;
  {	if (smdtab.b_active) {
	  if (++smdwticks >= SMDWLIM) {
	    smdwticks = 0;
	    printf("smd: lost interrupt\n");
	    smdintr();
	  }
	} else smdwticks = 0;
	timeout(smdwatch, &smdwstart, HZ);
}


/*-----------------------------------------------*/
/*  smdioctl - process disk io control commands. */
/*-----------------------------------------------*/
smdioctl(dev,cmd,addr,flag)
  dev_t dev;
  caddr_t addr;
  {	int track;

	switch (cmd) {
	  case DSKIOCFMT:
		/* format a disk track.  Argument is the track number to
		 * be formated.  Command can only be executed by superuser.
	 	 */
		if (u.u_uid != 0) {
		  u.u_error = EFAULT;
		  break;
		}
		spl6();		/* acquire use of raw i/o buffer */
		while (rsmdbuf.b_flags & B_BUSY) {
		  rsmdbuf.b_flags |= B_WANTED;
		  sleep((caddr_t)&rsmdbuf, PRIBIO+1);
		}
		rsmdbuf.b_flags = B_BUSY;
		rsmdbuf.b_dev = dev;
		rsmdbuf.b_error = 0;
		track_to_format = (int)addr;
		format_state = 0;	/* first format, then verify */

		/* call strategy routine at level 0 */
		spl0();
		smdstrategy(&rsmdbuf);
		spl6();

		/* loop until command has completed, then clean up */
		while (!(rsmdbuf.b_flags & B_DONE))
		  sleep((caddr_t)&rsmdbuf,PRIBIO);
		track_to_format = -1;
		if (rsmdbuf.b_flags & B_WANTED) wakeup((caddr_t)&rsmdbuf);
		rsmdbuf.b_flags &= ~(B_BUSY | B_WANTED);
		geterror(&rsmdbuf);
		spl0();
		return (u.u_error==0);

	  default:
		return(0);
	}

}
