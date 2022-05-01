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


#define	INOPB	(BSIZE/sizeof(struct dinode))


/*
 * Inode structure as it appears on a disk block.
 *
 * the 40 address bytes:
 *	39 used; 13 (NADDR) addresses of 3 bytes each.
 */

struct dinode {
	unsigned short	di_mode;     	/* mode and type of file */
	short	di_nlink;    	/* number of links to file */
	short	di_uid;      	/* owner's user id */
	short	di_gid;      	/* owner's group id */
	off_t	di_size;     	/* number of bytes in file */
	char  	di_addr[40];	/* disk block addresses */
	time_t	di_atime;   	/* time last accessed */
	time_t	di_mtime;   	/* time last modified */
	time_t	di_ctime;   	/* time created */
};


#define NADDR	13		/* number of block pointers in dinode */
#define	BLOCK	8		/* number of resolved block pointers saved */

struct inode {
	struct fsys *i_fsys;	/* pointer to filesys containing inode */
	char	i_flag;
	char	i_count;	/* reference count */
	dev_t	i_dev;		/* device where inode resides */
	ino_t	i_number;	/* i number, 1-to-1 with device address */
	unsigned short	i_mode;
	short	i_nlink;	/* directory entries */
	short	i_uid;		/* owner */
	short	i_gid;		/* group of owner */
	off_t	i_size;		/* size of file */
	time_t	i_atime;   	/* time last accessed */
	time_t	i_mtime;   	/* time last modified */
	time_t	i_ctime;   	/* time created */
	daddr_t i_addr[NADDR];	/* if normal file/directory */
	daddr_t	i_blocks;	/* first block whose pointer is in cache */
	daddr_t	i_block[BLOCK];	/* cache of block addresses (must be 2^n) */
};

struct	inode  *icache();
struct	inode  *ialloc();
hndl_t	       iopen();

#define	NULLINODE	((struct inode *)0)


/* flags */
#define	ILOCK	(1<<0)		/* inode is locked */
#define	IUPD	(1<<1)		/* file has been modified */
#define	IACC	(1<<2)		/* inode access time to be updated */
#define	IMOUNT	(1<<3)		/* inode is mounted on */
#define	IWANT	(1<<4)		/* some process waiting on lock */
#define	ITEXT	(1<<5)		/* inode is pure text prototype */
#define	ICHG	(1<<6)		/* inode has been changed */
#define	IINUSE	(1<<7)		/* inode cache slot is available for reuse */

/* modes */
#define	IFMT	0170000		/* type of file */
#define		IFDIR	0040000	/* directory */
#define		IFREG	0100000	/* regular */
#define		IFDEV	0020000 /* special device */
#define		IFCHR	0020000	/* character special */
#define		IFBLK	0060000	/* block special */
#define		IFMPC	0030000	/* multiplexed char special */
#define		IFMPB	0070000	/* multiplexed block special */
#define	ISUID	04000		/* set user id on execution */
#define	ISGID	02000		/* set group id on execution */
#define ISVTX	01000		/* save swapped text even after use */
#define	IREAD	0400		/* read, write, execute permissions */
#define	IWRITE	0200
#define	IEXEC	0100

#define	TRIXUID  1		/* user id for all files created under trix */
#define	TRIXGID  0		/* group id for all files created under trix */


#define	itoo(i)	((off_t)((i - 1) * sizeof(struct dinode) + 2 * BSIZE))
