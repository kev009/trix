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
#include "../h/unixtypes.h"
#include "../h/link.h"
#include	"param.h"
#include	"fsys.h"
#include	"inode.h"


#define	bcopy(FROM, TO, SIZE)						\
{									\
	register char	*p1 = (char *)FROM, *p2 = (char *)TO;		\
	register int	c;						\
	for (c = SIZE; --c >= 0; *p2++ = *p1++);			\
}


static badblock();


/*
 * balloc(fsys)  will return the block id of a free block on the
 *		 given filesystem
 */

daddr_t
balloc(fp)
register struct fsys  *fp;
{
	char	blk[BSIZE];
	daddr_t bn;

	/* lock free list manipulation */
	while(fp->fs_superb.s_flock)
		t_SLEEP(fsys_sync, &fp->fs_superb.s_flock);
	fp->fs_superb.s_flock++;

	/* find a block in free list */
	do {
		if(fp->fs_superb.s_nfree <= 0)
			goto nospace;
		if (fp->fs_superb.s_nfree > NICFREE) {
			fsyserr("Bad free count", fp);
			goto nospace;
		}
		bn = fp->fs_superb.s_free[--fp->fs_superb.s_nfree];
		if(bn == 0)
			goto nospace;
	} while (badblock(fp, bn));

	/* free list in super block empty, don't leave it that way */
	if(fp->fs_superb.s_nfree <= 0) {
		/* read in the next indirect block */
		if(t_READ(fp->fs_dev, (long)(bn<<BSHIFT), blk, BSIZE) != BSIZE)
		{	fsyserr("Can't read freeblock", fp);
			goto nospace;
		}
		bcopy(&blk[NFREE], &fp->fs_superb.s_nfree,
		     sizeof(fp->fs_superb.s_nfree));
		bcopy(&blk[LFREE], fp->fs_superb.s_free,
		     sizeof(fp->fs_superb.s_free));
		/*
		 * this should be conservative and write the sblock back with
		 * all these new blocks already allocated
		 */
		if(fp->fs_superb.s_nfree <=0)
			goto nospace;
	}

	/* free list has changed - write out superb on update */
	fp->fs_superb.s_tfree--;
	fp->fs_superb.s_fmod = 1;

	/* allow free list manipulation */
	fp->fs_superb.s_flock = 0;
	t_WAKEUP(fsys_sync, &fp->fs_superb.s_flock);

	/* initially block is full of zeros */
	if(t_WRITE(fp->fs_dev, (long)(bn<<BSHIFT), ZEROBLK, sizeof(ZEROBLK)) !=
	   sizeof(ZEROBLK))
		fsyserr("unable to clear allocated block", fp);
	return(bn);

nospace:
	/* allow free list manipulation */
	fp->fs_superb.s_flock = 0;
	t_WAKEUP(fsys_sync, &fp->fs_superb.s_flock);

	fp->fs_superb.s_nfree = 0;
	fsyserr("no space", fp);
	ERROR(E_SPACE);
	return(0);
}


/*
 * bfree(fsys, bn)  will return the block to the free list on the given
 *		    filesystem
 */

bfree(fp, bn)
struct fsys *fp;
daddr_t bn;
{
	char	blk[BSIZE];

	/* lock free list manipulation */
	while(fp->fs_superb.s_flock)
		t_SLEEP(fsys_sync, &fp->fs_superb.s_flock);
	fp->fs_superb.s_flock++;

	/* validate block being returned */
	if (badblock(fp, bn))
		panic("freeing badblock %x %D\n", fp, bn);

	if(fp->fs_superb.s_nfree <= 0) {
		fp->fs_superb.s_nfree = 1;
		fp->fs_superb.s_free[0] = 0;
	}


	/* free list in superb is full - write out indirect block */
	if(fp->fs_superb.s_nfree >= NICFREE) {
		bcopy(&fp->fs_superb.s_nfree, &blk[NFREE],
		     sizeof(fp->fs_superb.s_nfree));
		bcopy(fp->fs_superb.s_free, &blk[LFREE],
		     sizeof(fp->fs_superb.s_free));
		if(t_WRITE(fp->fs_dev, (long)(bn<<BSHIFT), blk, BSIZE)!=BSIZE)
			fsyserr("Can't write freeblock", fp);
		fp->fs_superb.s_nfree = 0;
	}

	/* finally free the block */
	fp->fs_superb.s_free[fp->fs_superb.s_nfree++] = bn;

	/* freelist has been modified - write out superb on update */
	fp->fs_superb.s_tfree++;
	fp->fs_superb.s_fmod = 1;

done:
	/* unlock free list manipulation */
	fp->fs_superb.s_flock = 0;
	t_WAKEUP(fsys_sync, &fp->fs_superb.s_flock);
}


/*
 * Check that a block number is in the range between the I list and the
 *	size of the device.
 * This is used mainly to check that a garbage file system is not mounted.
 */

static
badblock(fp, bn)
struct fsys *fp;
daddr_t bn;
{

	if (bn < fp->fs_superb.s_isize || bn >= fp->fs_superb.s_fsize) {
		fsyserr("bad block", fp);
		return(1);
	}
	return(0);
}


/*
 * block and bunlock  manage the problem of simultaneous access to device
 *		      blocks giving only one user access at a time
 * deadlocks are avoided by not being stupid about locking two things at once
 */

block(fp, bn)
register struct fsys  *fp;
	 daddr_t      bn;
{
	register int  i, fi;

search:
	/* look to see if block is already locked */
	for(i = 0, fi = -1 ; i < FS_LOCKS ; i++) {
		if(fp->fs_locks[i] == bn) {
printf("going to sleep on block(%d)\n", bn);
			t_SLEEP(fsys_sync, fp->fs_locks);
			goto search;
		}
		else
			if(fp->fs_locks[i] == 0)
				fi = i;
	}

	if(fi < 0) {
		/* wait for a lock to free up */
printf("block sleeping on lock space\n");
		t_SLEEP(fsys_sync, fp->fs_locks);
		goto search;
	}
printf("blocking(%d)\n", bn);
	fp->fs_locks[i] = bn;
}


bunlock(fp, bn)
register struct fsys  *fp;
	 daddr_t      bn;
{
	register int  i;

printf("bunlocking(%d)\n", bn);
	for(i = 0 ; i < FS_LOCKS ; i++) {
		if(fp->fs_locks[i] == bn) {
			fp->fs_locks[i] = 0;
			t_WAKEUP(fsys_sync, fp->fs_locks);
			return;
		}
	}
	panic("bunlock of unlocked block");
}
