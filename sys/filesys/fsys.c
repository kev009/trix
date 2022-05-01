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
#include "../h/unixtypes.h"
#include "../h/link.h"
#include	"param.h"
#include	"fsys.h"
#include	"inode.h"


static	struct	fsys	*fsalloc();

struct fsys  fsyss[N_FSYS];
struct inode inodes[N_INODE];


hndl_t
fsys_make()
{
	static	int	first = 1;
	extern  fsys_handler();
	extern	hndl_t sychndl;

	if(first) {
		struct fsys    *fp;
		struct sfenter *sfp;

		first = 0;

		/* free fstable */
		for(fp = fsyss ; fp < &fsyss[N_FSYS] ; fp++)
			fp->fs_dev = NULLHNDL;

		/* initialize softenter free list */
		sf_init();

		/* creat synchronizer for filesystem use */
		fsys_sync = t_DUP(sychndl);
	}
	return(t_MAKEPORT(fsys_handler, NULL, 0));
}


/*
 *  fsys_handler(fsys, mp) - handles requests to the filesystem
 */

fsys_handler(fp, mp)
register struct fsys     *fp;
	 struct message  *mp;
{

	switch(mp->m_opcode) {
	    case MOUNT:
	    {
		struct fsys   *fp;
		struct inode  *ip;

		/* creat a file system object for the given device */
		printf("in mount, about to fsalloc()\n");
		fp = fsalloc(mp->m_handle);
		ip = icache(fp, ROOTINO);
		mp->m_handle = iopen(ip);
		iunlock(ip);
/*		trix_enter(root, disk, "disk.mnt");*/
		if(mp->m_handle != NULLHNDL)
			mp->m_ercode = E_OK | OP_HANDLE;
		else
			mp->m_ercode = E_LOOKUP;
		return;
	    }

	    case UPDATE:
	    {
		struct fsys *fp;
		printf("FSYS: UPDATE\n");
		for(fp = fsyss ; fp < &fsyss[N_FSYS] ; fp++)
			if(fp->fs_dev != NULLHNDL)
				fsupdate(fp);

		break;
	    }

	    case CLOSE:
		break;

	    default:
		mp->m_ercode = E_OPCODE;
		return;
	}

	mp->m_ercode = E_OK;
}


/*
 *  fsalloc() and fsfree() allocate and free the fsys struct of a mounted
 *	filesystem
 */

static
struct fsys *
fsalloc(dev)
hndl_t  dev;
{
	struct fsys *fp;

	/* allocate a file system object */
	for(fp = fsyss ; fp < &fsyss[N_FSYS] ; fp++)
		if(fp->fs_dev == NULLHNDL) {
			/* read in super block */
			t_READ(dev,
			  (long)(SUPERB<<BSHIFT),
			  &fp->fs_superb,
			  sizeof(fp->fs_superb));
			/* initialize fsys data */
			printf("fsaloc(%s:%s) is=%d fs=%d nf=%d fl=%d il=%d\n",
				fp->fs_superb.s_fname,
				fp->fs_superb.s_fpack,
				fp->fs_superb.s_isize,
				fp->fs_superb.s_fsize,
				fp->fs_superb.s_nfree,
				fp->fs_superb.s_flock,
				fp->fs_superb.s_ilock);
			fp->fs_dev = dev;
			fp->fs_opncnt = 0;
			fp->fs_superb.s_flock = fp->fs_superb.s_ilock = 0;
			fp->fs_superb.s_fmod = 0;

			/* update system clock if necessary */
			{
				extern hndl_t	clkhndl;
				int	ctime;

				t_READ(clkhndl, 0, &ctime, sizeof(ctime));
				if(ctime < fp->fs_superb.s_time)
				t_WRITE(clkhndl,
					0,
					&fp->fs_superb.s_time,
					sizeof(ctime));
			}

			/* init list of softenteres */
			l_init(&fp->fs_mntd);

			return(fp);
		}
	panic("fsyss table full");
}


fsfree(fp)
register struct fsys  *fp;
{
	struct link *l;
	struct sfenter *sf;
	/* free all softentered objects */
	for(l = fp->fs_mntd.next; l != NULLLINK; l = l->next){
		sf = LINKREF(l, sf_next, struct sfenter *);
		if(sf == (struct sfenter *)0) continue;
		t_CLOSE(sf->sf_hndl);
		l_unlink(&sf->sf_next);
		sf_free(sf);
	}

	
	ASSERT(fp->fs_superb.s_ilock==0 && fp->fs_superb.s_flock==0);

	/* flush any incore information dealing with filesystem */
	fsupdate(fp);
	/* close device */
	t_CLOSE(fp->fs_dev);

	/* remove the filesystem from the fsyss table */
	fp->fs_dev = NULLHNDL;
}


/*
 *  fsupdate(fsys) - flush out in core information for filesystem
 */

fsupdate(fp)
register struct fsys  *fp;
{
	struct inode *ip;

    /* until softenters exist, always update ALL the filesystems */
    for(fp = fsyss ; fp < &fsyss[N_FSYS] ; fp++) {
	if(fp->fs_dev == NULLHNDL)
		continue;
	/* update super block */
	if(fp->fs_superb.s_fmod != 0 && fp->fs_superb.s_ilock == 0 &&
	   fp->fs_superb.s_flock == 0 && fp->fs_superb.s_ronly == 0) {
		fp->fs_superb.s_fmod = 0;
		t_READ(clkhndl, 0, &fp->fs_superb.s_time, sizeof(long));
		t_WRITE(fp->fs_dev,
			(long)(SUPERB<<BSHIFT),
			&fp->fs_superb,
			sizeof(fp->fs_superb));
	}

	/* update inode info for each open file */
	for(ip = &inodes[0] ; ip < &inodes[N_INODE] ; ip++)
		if(ip->i_fsys == fp)
			iupdat(ip);

	/* should send update to softmounted inodes */

	/* send update to device driver to force data out of cache */
	t_UPDATE(fp->fs_dev);
    }
}


/*
 *  fsyserr(message, fsys) - print an error message for the file system
 */

fsyserr(m, fp)
char	     *m;
struct fsys  *fp;
{
	printf("fsyserr: %s (%o)\n", m, fp);
}
