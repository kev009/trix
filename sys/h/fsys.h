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


#define	SUPERB	1		/* block number of superblock */

#define FREEINO	0		/* inumber of empty directory entries */
#define	SOFTINO	1		/* inumber of ALL soft enters */
#define ROOTINO	2		/* inumber of root (1 is for badblocks) */

#define NICFREE	178		/* number of superblock free blocks */
#define NICINOD 100		/* number of superblock inodes */

#define BMASK	((1<<BSHIFT)-1)	/* mask for converting byte offset to block */
#define BSIZE	(1<<BSHIFT)	/* size of block */

#define	NINDIR	(BSIZE/sizeof(daddr_t))	/* number of pointers in iblock */
#define	NSHIFT	(BSHIFT - 2)
#define NMASK	((1<<NSHIFT)-1)

#define NFREE	0		/* offset of freecount into freelist block */
#define LFREE	sizeof(short)	/* offset of freelist into freelist block */


#define	FS_LOCKS	10


struct fsys {
	int	fs_opncnt;		/* incore files on filesystem */
	hndl_t	fs_dev;			/* device of mounted filesystem */
	ino_t	fs_locks[FS_LOCKS];	/* locking on device blocks */
	struct  list	 fs_mntd;	/* list of handles of softenters */

	struct {
		unsigned short s_isize;	/* size in blocks of i-list */
		daddr_t	s_fsize;   	/* size in blocks of entire volume */
		short  	s_nfree;   	/* number of addresses in s_free */
		daddr_t	s_free[NICFREE];/* free block list */
		short  	s_ninode;  	/* number of i-nodes in s_inode */
		ino_t  	s_inode[NICINOD];/* free i-node list */
		char   	s_flock;   	/* lock for free list manipulation */
		char   	s_ilock;   	/* lock for i-list manipulation */
		char   	s_fmod;    	/* super block modified flag */
		char   	s_ronly;   	/* mounted read-only flag */
		time_t 	s_time;    	/* last super block update */
		/* remainder not maintained by this version of the system */
		daddr_t	s_tfree;   	/* total free blocks*/
		ino_t  	s_tinode;  	/* total free inodes */
		short  	s_m;       	/* interleave factor */
		short  	s_n;       	/* " " */
		char   	s_fname[6];	/* file system name */
		char   	s_fpack[6];	/* file system pack name */
	} fs_superb;		/* super block of filesystem */
};


#define NAMESIZE 14		/* size of filename segment in directory */

#define	DSIZE	(sizeof(struct direct))
#define	DMASK	(DSIZE - 1)

struct direct {
	ino_t	d_ino;
	char	d_name[NAMESIZE];
};


#define	SFENTER	25

struct sfenter {
	struct	list	sf_next;	/* next softenter in list */
	ino_t		sf_direct;	/* directory containing entry */
	char		sf_name[NAMESIZE]; /* name softentered in directory */
	hndl_t		sf_hndl;	/* handle softentered */
}	sfenter[SFENTER];

struct list sffree;


char	ZEROBLK[BSIZE];
hndl_t	fsys_sync;
