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


#include "../h/param.h"
#include "../h/args.h"
#include "../h/calls.h"
#include "../h/assert.h"
#include "../h/link.h"
#include "../h/status.h"
#include "../h/protocol.h"
#include "../h/unixtypes.h"
#include	"param.h"
#include	"fsys.h"
#include	"inode.h"


static	int	bloop();
struct	inode	inodes[N_INODE];
int	icachehits;
int	icachemiss;


#define	min(x,y)	((x) < (y) ? (x) : (y))


/*
 *  return a pointer to the incore copy of the designated inode
 *    (reading it in if necessary)
 *  the inode's usage is incremented and it is locked
 */

struct inode *
icache(fp, ino)
register struct	fsys	*fp;
register ino_t		ino;
{
	register struct inode	*ip;
		 struct	inode	*fip = NULLINODE;
		 struct dinode	dino;
	register struct dinode	*dp;
	static	 struct	inode	*isearch = &inodes[0];

search:
	for(ip = isearch ; ip < &inodes[N_INODE] ; ip++) {
		if(ip->i_fsys == fp && ino == ip->i_number) {
			/* we found it in core */
			if((ip->i_flag&ILOCK) != 0) {
				/* its locked */
				DEBUGOUT(("sleeping on locked %d in ichache\n",
					ip->i_number));
				ip->i_flag |= IWANT;
				t_SLEEP(fsys_sync, ip);
				/* better look again in case its gone now */
				goto search;
			}

			if(!(ip->i_flag & IINUSE)) {
				/* a real hit -- the file is not currently open */
				icachehits++;
				++fp->fs_opncnt;
			}

			/* its ours for now */
			ip->i_count++;
			ip->i_flag |= ILOCK|IINUSE;
			return(ip);
		}

		/* remember a free inode */
		if(fip == NULLINODE && !(ip->i_flag & IINUSE))
			fip = ip;
	}

	if(isearch != &inodes[0]) {
		/* no match from the search point -- try again from the beginning */
		isearch = &inodes[0];
		goto search;
	}

	if(fip == NULLINODE) {
		/* searched from the beginning and there were no free slots */
		panic("no free inode slots");
		/* should actually sleep and retry */
	}

	/* continue search from here next time */
	isearch = &fip[1];

	/* inode is not in core -- allocate one and read it in */
	icachemiss++;
	ip = fip;
	++fp->fs_opncnt;
	ip->i_fsys = fp;
	ip->i_number = ino;
	ip->i_flag = ILOCK|IINUSE;
	ip->i_count = 1;
	ip->i_blocks = -BLOCK;	/* don't match on the cache */
	if(t_READ(fp->fs_dev,
		  (off_t)itoo(ino),
		  (addr_t)(&dino),
		  (off_t)sizeof(dino)) != sizeof(dino)) {
			/* read got error */
			panic("read error in icache");
			return(NULLINODE);
	}

	/* convert from the on disk format to incore format */
	dp = &dino;
	ip->i_mode = dp->di_mode;
	ip->i_nlink = dp->di_nlink;
	ip->i_uid = dp->di_uid;
	ip->i_gid = dp->di_gid;
	ip->i_size = dp->di_size;
	ip->i_atime = dp->di_atime;
	ip->i_ctime = dp->di_ctime;
	ip->i_mtime = dp->di_mtime;
	l3tol((char *)ip->i_addr, (char *)dp->di_addr, NADDR);
	DEBUGOUT((" inum:%d mode:%o nlnk:%d uid:%d gid:%d size:%d\n",
		ip->i_number,ip->i_mode, ip->i_nlink,
		ip->i_uid, ip->i_gid, ip->i_size));
	return(ip);
}


/*
 *  allocate an unused inode on the given filesystem and make it the
 *    given mode (for file creation)
 *  the inode is returned in the state left by icache (usage=1 and locked)
 *    and it has no links
 *  the algorithm keeps up to NICINOD spare I nodes in the super block
 *  when this runs out, a linear search through the I list is instituted
 *    to pick up NICINOD more
 *  the inodes in the list are not assumed to actually be free
 */

struct inode *
ialloc(fp, mode)
register struct	fsys  *fp;
{
	register struct inode	*ip;
		 struct dinode	dino[BSIZE/sizeof(struct dinode)];
		 int 		i;
		 ino_t		ino;

	/* wait for filesystem to be idle */
	while(fp->fs_superb.s_ilock) {
		printf("sleeping on ilock in ialloc\n");
		t_SLEEP(fsys_sync, fp);
	}

loop:
	if(fp->fs_superb.s_ninode > 0) {
		/* get an inode from the free inode table in sblock */
		ino = fp->fs_superb.s_inode[--fp->fs_superb.s_ninode];
		ip = icache(fp, ino);
		if(ip == NULLINODE)
			return(NULLINODE);

		if(ip->i_mode == 0) {
			/* make sure its really free */
			ip->i_size = 0;
			for (i=0; i<NADDR; i++)
				ip->i_addr[i] = 0;
			ip->i_blocks = -BLOCK;	/* don't match on the cache */
			ip->i_nlink = 0;
			ip->i_flag |= IACC|IUPD|ICHG;
			ip->i_uid = TRIXUID;
			ip->i_gid = TRIXGID;
			/* asking for unallocated type -> regular file */
			if((mode & IFMT) == 0)
				mode |= IFREG;
			ip->i_mode = mode;
			fp->fs_superb.s_tinode--;
			fp->fs_superb.s_fmod = 1;
			return(ip);
		}

		/* it was allocated after all,  look some more */
		ifree(ip);
		goto loop;
	}

	/* get a "bunch" of free inodes */
	fp->fs_superb.s_ilock++;
	/* this should really search from where it last stopped ... */
	for(ino = 1 ; itoo(ino)>>BSHIFT < fp->fs_superb.s_isize ; ) {
		if(t_READ(fp->fs_dev,
			  (off_t)itoo(ino),
			  (addr_t)(dino),
			  (off_t)sizeof(dino)) != sizeof(dino)) {
				/* read got error */
				panic("read error in icache");
				return(NULLINODE);
		}
		for(i = 0 ; i < sizeof(dino)/sizeof(dino[0]) ; i++, ino++) {
			if(dino[i].di_mode == 0)
				fp->fs_superb.s_inode[fp->fs_superb.s_ninode++] = ino;
			if(fp->fs_superb.s_ninode >= NICINOD)
				goto done;
		}
	}

done:
	fp->fs_superb.s_ilock = 0;
	t_WAKEUP(fsys_sync, fp);
	if(fp->fs_superb.s_ninode > 0)
		goto loop;

	fsyserr("Out of inodes", fp);
	return(NULLINODE);
}


/*
 *  free up a use of the incore copy of the designated inode
 *    (writing it out if it is no longer needed)
 *  if it is no longer active (usage=0) and there are no links to it free it
 *    on disk
 *  the inode is left unlocked with its usage decremented
 */

ifree(ip)
register struct inode  *ip;
{
	if(ip->i_count == 1) {
		register struct	fsys  *fp;

		/* this is the last active use */
		fp = ip->i_fsys;
		ip->i_flag |= ILOCK;

		if(ip->i_nlink <= 0) {
			/* the inode is really free (no links to it) */
			ip->i_nlink = 0;
			itrunc(ip);
			ip->i_mode = 0;
			ip->i_flag |= IUPD|ICHG;
			if(!fp->fs_superb.s_ilock &&
			  fp->fs_superb.s_ninode < NICINOD) {
				/* remember the free inode */
				fp->fs_superb.s_inode[fp->fs_superb.s_ninode++]
				  = ip->i_number;
			}
			fp->fs_superb.s_tinode++;
			fp->fs_superb.s_fmod = 1;
		}

		/* update the disk copy before freeing the cached version */
		iupdat(ip);

		/* mark the slot as free */
		ip->i_flag &= ~IINUSE;
		ip->i_count--;
		iunlock(ip);

		/* if this is the last open file on the filesystem, unmount it */
		if(--fp->fs_opncnt <= 0) {
			printf("FREEING FILESYSTEM (inum=%d)\n", ip->i_number);
			fsfree(fp);
		}
	}
	else {
		/* its still actively used */
		ip->i_count--;
		iunlock(ip);
	}
}


/*
 *  free all the disk blocks associated with the specified inode structure
 *  the blocks of the file are removed in reverse order
 *  this FILO algorithm will tend to maintain a contiguous free list
 *    longer than FIFO.
 */

itrunc(ip)
register struct inode  *ip;
{
	register	  i;
	register daddr_t  bn;

	i = itype(ip);
	if(i != IFREG && i != IFDIR)
		return;

	/* free all the blocks associated with a file or directory */
	for(i = NADDR-1 ; i >= 0 ; i--) {
		bn = ip->i_addr[i];
		if(bn == (daddr_t)0)
			continue;

		ip->i_addr[i] = (daddr_t)0;

		switch(i) {
		    default:
			/* direct blocks */
			bfree(ip->i_fsys, bn);
			break;
		    case NADDR-3:
			/* indirect blocks */
			bloop(ip, bn, 0);
			break;
		    case NADDR-2:
			/* double indirect blocks */
			bloop(ip, bn, 1);
			break;
		    case NADDR-1:
			/* triple indirect blocks */
			bloop(ip, bn, 2);
			break;
		}
	}

	ip->i_size = 0;
	ip->i_flag |= ICHG|IUPD;
	ip->i_blocks = -BLOCK;	/* don't match on the cache */
}


static
bloop(ip, bn, lev)
register struct	inode	*ip;
	 daddr_t      bn;
{
	register struct fsys  *fp = ip->i_fsys;
	register int      i;

	/* loop over an indirect block and free the tree of blocks */
	if(lev) {
		daddr_t  nb;

		for(i = 0 ; i < NINDIR ; i++) {
			/* read in a single block number to avoid needing a buffer */
			t_READ(fp->fs_dev,
			  (long)((bn<<BSHIFT) + i*sizeof(nb)),
			  &nb,
			  sizeof(nb));

			if(nb != (daddr_t)0)
				bloop(ip, nb, lev-1);
		}
	}
	else {
		int	j;

		for(i = 0 ; i < NINDIR ; i += BLOCK) {
			/* read into cache to reduce disk traffic */
			if(t_READ(ip->i_fsys->fs_dev,
				  (long)((bn<<BSHIFT) + i*sizeof(bn)),
				  ip->i_block,
				  sizeof(ip->i_block)) != sizeof(ip->i_block)){
				printf("can't read into icache (%d) in bloop\n", i);
				continue;
			}

			for(j = 0 ; j < BLOCK ; j++) {
				if(ip->i_block[j] != (daddr_t)0)
					bfree(fp, ip->i_block[j]);
			}
		}
	}

	/* free the indirect block */
	bfree(fp, bn);
}


/*
 *  update the disk copy of the inode with the data from the incore copy
 *  check accessed and update flags on an inode structure
 *  if any are on, update the inode with the current time
 */

iupdat(ip)
register struct inode  *ip;
{
		 time_t        time;
		 struct dinode dino;
	register struct dinode *dp = &dino;

	if((ip->i_flag&(IUPD|IACC|ICHG)) != 0) {
		/* its been changed */
		if(ip->i_fsys->fs_superb.s_ronly)
			/* the filesystem is not mounted for writing */
			return;

		/* read the disk copy in to get unchanged information */
		if(t_READ(ip->i_fsys->fs_dev,
			  (off_t)itoo(ip->i_number),
			  (addr_t)dp,
			  (off_t)sizeof(dino)) != sizeof(dino))
				panic("write error in iupdat");

		/* convert incore format to disk format */
		dp->di_mode = ip->i_mode;
		dp->di_nlink = ip->i_nlink;
		dp->di_uid = ip->i_uid;
		dp->di_gid = ip->i_gid;
		dp->di_size = ip->i_size;

		{
			extern hndl_t	clkhndl;
			t_READ(clkhndl, 0, &time, sizeof(time));
			if(ip->i_flag&IACC)
				ip->i_atime = time;
			if(ip->i_flag&ICHG)
				ip->i_ctime = time;
			if(ip->i_flag&IUPD)
				ip->i_mtime = time;
		}
		dp->di_atime = ip->i_atime;
		dp->di_ctime = ip->i_ctime;
		dp->di_mtime = ip->i_mtime;

		if(ltol3((char *)dp->di_addr,
		    (char *)ip->i_addr,
		    NADDR) != NADDR)
			DEBUGOUT(("iaddress > 2^24 (ino=%d)\n",
			  ip->i_number));

		ip->i_flag &= ~(IUPD|IACC|ICHG);

		if(t_WRITE(ip->i_fsys->fs_dev,
			   (off_t)itoo(ip->i_number),
			   (addr_t)dp,
			   (off_t)sizeof(dino)) != sizeof(dino))
				panic("write error in iupdat");
	}
}


/*
 *  returns a port to a handler for the given inode
 *  the inode is unmodified
 */

hndl_t
iopen(ip)
register struct inode  *ip;
{
	extern void  stat_handler();
	extern void  drct_handler();
	extern void  file_handler();

	switch(itype(ip)) {
 	   case IFREG:
		return(t_MAKEPORT(file_handler, ip, 0));
	    case IFDIR:
		return(t_MAKEPORT(drct_handler, ip, 0));
	    default:
		printf("Bad inode (%d) type (%o)", ip->i_number, itype(ip));
		return(t_MAKEPORT(stat_handler, ip, 0));
	}
}


/*
 *  read count bytes of data from the file designated by inode
 *    into data (which can either be local or the passed data window)
 *  it uses t_READA so it can designate a read ahead block
 */

off_t
iread(ip, offset, count, data)
struct inode *ip;
off_t  offset;
off_t  count;
addr_t data;
{
	off_t    rcnt, cnt;
	daddr_t  bn, rabn;
	off_t    on, n;

	DEBUGOUT(("iread(%d,%d,%d,%x)\n", ip->i_number, offset, count, data));
	if(offset < 0) {
/*		ERROR(EOFFSET);*/
		return(0);
	}

	rcnt = 0;
	while(count > 0) {
		/* don't read past end of file */
		if((n = ip->i_size - offset) <= 0)
			break;

		/* find block on device to read */
		on = offset & BMASK;
		n = min(n, min((unsigned)(BSIZE-on), count));

		/* compute actual block to read */
		bn = imap(ip, offset>>BSHIFT, READ);
		if(bn == 0 || bn == -1) {
			printf("filling read hole on block %d inode %d\n",
				offset>>BSHIFT, ip->i_number);
			bn = imap(ip, offset>>BSHIFT, WRITE);
		}
		if(bn == 0 || bn == -1) {
			printf("m=%o s=%d a=%d,%d,%d\nb=%d,%d,%d\n",
				ip->i_mode, ip->i_size, ip->i_addr[0],
				ip->i_addr[1], ip->i_addr[2], ip->i_block[0],
				ip->i_block[1], ip->i_block[2]);
			panic("imap(%d,%d) -> %d\n",
				ip->i_number, offset>>BSHIFT, bn);
		}

		/* find length of contiguous read and compute read ahead block */
		rabn = bn;
		i = 1;
		do {
			if((rabn = imap(ip, (offset>>BSHIFT) + i, READ) != bn + i)
				break;
		}  while(n + (++i<<BSHIFT) <= count);
		n = min(n + (i<<BSHIFT), count);

		if(rabn != 0 && rabn != -1) {
			/* valid read ahead - reada the data in */
			cnt = t_READA(ip->i_fsys->fs_dev,
				      (off_t)((bn<<BSHIFT) + on),
				      (addr_t)(data + rcnt),
				      (off_t)n,
				      (off_t)(rabn<<BSHIFT) );
		}
		else {
			/* no read ahead - simply read the data in */
			cnt = t_READ(ip->i_fsys->fs_dev,
				     (off_t)((bn<<BSHIFT) + on),
				     (addr_t)(data + rcnt),
				     (off_t)n );
		}

		/* an error? */
		if(cnt <= 0) {
			printf("iread returns %x\n", cnt);
			panic("read error in iread");
			break;
		}

		rcnt += cnt;
		count -= cnt;
		offset += cnt;
		ip->i_flag |= IACC;
	}

#ifdef SOFTENTERS
	if(count > 0 && (ip->i_mode&IFMT) == IFDIR) {
		struct mount *mp;

		/* special handling of directories to catch soft enters */
		offset -= ip->i_size;
		for(mp = ip->i_fsys->fs_mntd ; mp != NULL ; mp = mp->mn_nxt) {
			struct direct d;

			if(mp->mn_ino != ip->i_number)
				continue;
			if(offset >= DSIZE) {
				offset -= DSIZE;
				continue;
			}
			d.d_ino = SOFTINO;
			strncpy(d.d_name, mp->mn_name, NAMESIZE);
			on = offset & DMASK;
			n = min(DSIZE-on, count);
			cnt = Store((char *)(data.pntr+rcnt), &d, n);
			if(cnt <= 0)
				break;
			rcnt += cnt;
			count -= cnt;
			offset = (offset+cnt) & DMASK;
			ip->i_flag |= IACC;
		}
	}
#endif
	return(rcnt);
}


/*
 *  write count bytes of data to the file designated by inode
 *    into data (which can either be local or the passed data window)
 */

off_t
iwrite(ip, offset, count, data)
struct inode *ip;
off_t  offset;
off_t  count;
addr_t data;
{
	off_t	rcnt, cnt;
	daddr_t	bn;
	off_t	on, n;

	DEBUGOUT(("iwrite(%d,%d,%d,%x)\n", ip->i_number, offset, count, data));
	if(offset == APPEND_OFF)
		offset = ip->i_size;
	if(offset < 0) {
/*		ERROR(EOFFSET);*/
		return(0);
	}

	rcnt = 0;
	while(count > 0) {
		/* find block on device to read */
		on = offset & BMASK;
		n = min((unsigned)(BSIZE-on), count);
		bn = imap(ip, offset>>BSHIFT, WRITE);

		/* write the data out */
		cnt = t_WRITE(ip->i_fsys->fs_dev,
			      (off_t)((bn<<BSHIFT) + on),
			      (addr_t)(data + rcnt),
			      (off_t)n );

		/* an error? */
		if(cnt <= 0) {
			panic("write error in iwrite");
			break;
		}

		rcnt += cnt;
		count -= cnt;
		offset += cnt;
		if(offset > ip->i_size) {
			DEBUGOUT(("extending file\n"));
			ip->i_size = offset;
		}
		ip->i_flag |= IUPD|ICHG;
	}
	return(rcnt);
}


/* GETSTAT on inodes */

long
igstat(ip, type, dflt)
register struct inode  *ip;
int	 type;
long	 dflt;
{
	switch(type) {
	    case STATUS_ATIME:
		iupdat(ip);
		return(ip->i_atime);

	    case STATUS_CTIME:
		iupdat(ip);
		return(ip->i_ctime);

	    case STATUS_MTIME:
		iupdat(ip);
		return(ip->i_mtime);

	    case STATUS_IDENT:
		return(ip->i_number | ((int)ip->i_fsys << 16));

	    case STATUS_PROT:
		switch(itype(ip)) {
		    case IFREG:
			return(PROT_FILE);
		    case IFDIR:
			return(PROT_DIRECT);
		}
		return(PROT_UNKNOWN);

	    case STATUS_ACCESS:
		return(ip->i_mode & 0777);

	    case STATUS_OWNER:
		return((ip->i_uid) | (ip->i_gid << 16));

	    case STATUS_SUSES:
		return(ip->i_nlink);

	    case STATUS_ISIZE:
		/* should include size of softenters */
		return(ip->i_size);
	}

	return(dflt);
}


/*  PUTSTAT on inode */

long
ipstat(ip, type, val)
register struct inode  *ip;
int	 type;
long	 val;
{
	switch(type) {
	    case STATUS_ACCESS:
		val &= 07777;
		ip->i_mode &= ~07777;
		ip->i_mode |= val;
		break;

	    case STATUS_OWNER:
		ip->i_uid = val & 0xFFFF;
		ip->i_gid = (val >> 16) & 0xFFFF;
		break;

	    case STATUS_ATIME:
		ip->i_atime = val;
		break;

	    case STATUS_MTIME:
		ip->i_mtime = val;

	    default:
		return(-1);
	}

	ip->i_flag |= ICHG;
	return(val);
}


/* return the type of the inode */

itype(ip)
struct inode  *ip;
{
	return(ip->i_mode & IFMT);
}


/* wait for an inode to be unlocked and lock it */

ilock(ip)
struct inode  *ip;
{
	/* you must have a valid (counted) pointer to the inode */
	while(ip->i_flag & ILOCK) {
		DEBUGOUT(("sleeping in ilock on %d\n", ip->i_number));
		/* mark the inode as wanted so unlock will wake us up */
		ip->i_flag |= IWANT;
		t_SLEEP(fsys_sync, ip);
	}
	ip->i_flag |= ILOCK;
}


/* increment the inodes use count (the inode is assued to be locked by the caller) */

iuse(ip)
register struct	inode  *ip;
{
	ASSERT(ip->i_flag & ILOCK);
	ip->i_count++;
}

 	
/* unlock an inode and wakeup those waiting for it */

iunlock(ip)
register struct inode  *ip;
{
	ip->i_flag &= ~ILOCK;
	if(ip->i_flag & IWANT) {
		/* only do wakeup if someone is waiting */
		ip->i_flag &= ~IWANT;
		t_WAKEUP(fsys_sync, ip);
	}
}


/* map virtual blocks in a file to physical disk blocks */

daddr_t
imap(ip, bn, rwflg)
register struct inode *ip;
register daddr_t bn;
{
	register i;
	int j, sh;
	daddr_t nb, nnb;
	daddr_t rbn;

	if(bn < 0) {
/*		ERROR(EFSIZE);*/
		return((daddr_t)0);
	}

	/*
	 * blocks 0..NADDR-4 are direct blocks
	 */
	if(bn < NADDR-3) {
		i = bn;
		nb = ip->i_addr[i];
		if(nb == 0) {
			if(rwflg==READ || (nb = balloc(ip->i_fsys)) == 0)
				return((daddr_t)-1);
			ip->i_addr[i] = nb;
			ip->i_flag |= IUPD|ICHG;
		}
		return(nb);
	}

	/*
	 * check to see if the block is in the blocks cache
	 * if it is we dont have to go to the disk
	 * this should make a BIG difference in large files
	 */
	if(bn >= ip->i_blocks && bn < ip->i_blocks+BLOCK &&
	   ip->i_block[bn - ip->i_blocks] != 0)
		return(ip->i_block[bn - ip->i_blocks]);
	/* save the real block number */
	rbn = bn;

	/*
	 * addresses NADDR-3, NADDR-2, and NADDR-1
	 * have single, double, triple indirect blocks.
	 * the first step is to determine
	 * how many levels of indirection.
	 */
	sh = 0;
	nb = 1;
	bn -= NADDR-3;
	for(j=3; j>0; j--) {
		sh += NSHIFT;
		nb <<= NSHIFT;
		if(bn < nb)
			break;
		bn -= nb;
	}
	if(j == 0) {
		ERROR(E_MAXSIZE);
		return((daddr_t)0);
	}

	/*
	 * fetch the address from the ip
	 */
	nb = ip->i_addr[NADDR-j];
	if(nb == 0) {
		if(rwflg==READ || (nb = balloc(ip->i_fsys)) == 0)
			return((daddr_t)-1);
		ip->i_addr[NADDR-j] = nb;
		ip->i_flag |= IUPD|ICHG;
	}

	/*
	 * fetch through the indirect blocks
	 */
	for(; j<=3; j++) {
		sh -= NSHIFT;
		i = (bn>>sh) & NMASK;
		if(j == 3) {
			/*
			 * last level of indirection
			 * read info into the blocks cache
			 */
			ip->i_blocks = rbn - (i & (BLOCK-1));
			if(t_READ(ip->i_fsys->fs_dev,
				  (long)((nb<<BSHIFT) +
					 ((i & ~(BLOCK-1))*sizeof(nb))),
				  ip->i_block,
				  sizeof(ip->i_block))!=sizeof(ip->i_block)){
					/* invalidate the cache */
					ip->i_blocks = -BLOCK;
					return((daddr_t)0);
			}
			nnb = ip->i_block[rbn - ip->i_blocks];
		}
		else {
			/* get the single indirect block entry */
			if(t_READ(ip->i_fsys->fs_dev,
				  (long)((nb<<BSHIFT) + (i*sizeof(nb))),
				  &nnb,
				  sizeof(nb)) != sizeof(nb))
					return((daddr_t)0);
		}

		if(nnb == 0) {
			if(rwflg==READ || (nnb = balloc(ip->i_fsys)) == 0)
				return((daddr_t)-1);
			t_WRITE(ip->i_fsys->fs_dev,
				(long)((nb<<BSHIFT) + (i*sizeof(nb))),
				&nnb,
				sizeof(nb));
			/* invalidate cache (in case we do a read) */
			ip->i_blocks = -BLOCK;
		}
		nb = nnb;
	}
	return(nb);
}
