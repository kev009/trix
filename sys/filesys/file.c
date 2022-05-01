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


#define	DEBUG	DEBUG
#include "../h/param.h"
#include "../h/args.h"
#include "../h/calls.h"
#include "../h/protocol.h"
#include "../h/assert.h"
#include "../h/link.h"
#include "../h/unixtypes.h"
#include	"param.h"
#include	"fsys.h"
#include	"inode.h"

extern	struct	sfenter	*sf_lookup();
extern	struct	sfenter	*sf_alloc();
extern	long	igstat();
extern	long	ipstat();


/*
 *  file_handler() and drct_handler() are handlers for files and directories
 */


file_handler(ip, mp)
struct inode	*ip;
struct message	*mp;
{
	DEBUGOUT(("file_handler: %x\n", mp->m_opcode));

	ilock(ip);
	switch(mp->m_opcode) {
	    case READ:
		mp->m_param[0] = 
		    iread(ip, mp->m_param[0], mp->m_dwsize, mp->m_dwaddr);
		break;

	    case WRITE:
		mp->m_param[0] = 
		    iwrite(ip, mp->m_param[0], mp->m_dwsize, mp->m_dwaddr);
		break;

	    case CLOSE:
		ifree(ip);
		mp->m_ercode = E_OK;
		return;

	    case GETSTAT:
	    case PUTSTAT:
		iunlock(ip);
		file_stat(ip, mp);
		return;

	    case CONNECT:
		if(itype(ip) == IFREG &&
		   mp->m_param[0] == CONNECT_TRUNC)
			itrunc(ip);
		iunlock(ip);
		mp->m_ercode = E_ASIS;
		return;

	    case UPDATE:
		if(ip->i_number == ROOTINO) {
			/* update to root does sync of file system */
			fsupdate(ip->i_fsys);
		}
		else {
			/* update single file */
			iupdat(ip);
			/* for now do update on filesys */
			/* eventually iupdat will force out super block */
			fsupdate(ip->i_fsys);
		}
		break;

	    default:
		DEBUGOUT(("bad file op: %x\n", mp->m_opcode));
		iunlock(ip);
		mp->m_ercode = E_OPCODE;
		return;
	}

	iunlock(ip);
	mp->m_ercode = E_OK;
}


drct_handler(ip, mp)
struct inode *ip;
struct message *mp;
{
	DEBUGOUT(("drct_handler: %x\n", mp->m_opcode));

	ASSERT((ip->i_mode & IFMT) == IFDIR);
	switch(mp->m_opcode) {
	    case CREAT:
printf("file CREAT\n");
		/* creat a new object of the given type */
		ip = ialloc(ip->i_fsys, mp->m_param[0]);
		if(ip != NULLINODE) {
			/*
			 *  the inode is locked and usage=1
			 *  create a port referencing it and unlock it
			 *  the inode stays around because usage>0
			 */
			mp->m_handle = iopen(ip);
			iunlock(ip);
			if(mp->m_handle != NULLHNDL)
				mp->m_ercode = E_OK|OP_HANDLE;
			else
				ERROR(E_LOOKUP);
		}
		else
			ERROR(E_SPACE);
		return;

	    case LOOKUP:
		drct_lookup(ip, mp);
		return;

	    case ENTER:
		drct_enter(ip, mp, 0);
		return;

	    case SOFTENTER:
		drct_enter(ip, mp, 1);
		return;

	    case DELETE:
		drct_delete(ip, mp);
		return;

	    case WRITE:
		mp->m_ercode = E_OPCODE;
		return;

	    default:
		file_handler(ip, mp);
		return;
	}
}


/*
 *  stat_handler() is the object interfaces to files of unknown type.
 *	it is there to allow GETSTAT and DELETE on strange inodes.
 */

stat_handler(ip, mp)
struct inode	*ip;
struct message	*mp;
{

	switch(mp->m_opcode) {
	    case CLOSE:
	    case GETSTAT:
		file_handler(ip, mp);
		return;

	    default:
		printf("bad stat op: %x\n", mp->m_opcode);
		mp->m_ercode = E_OPCODE;
		return;
	}
}


/*
 *  search the directory for the name designated by the message window and
 *    do a GETSTAT/PUTSTAT on the associated object
 *  the name is parsed until there is:
 *    none left
 *    no match (in which case you return an error)
 *    a nonlocal enter (in which case t_JUMP is used to continue the STAT)
 *  the file inode should be unused
 */

file_stat(dir, mp)
register struct inode    *dir;
register struct message  *mp;
{
	char	c, fname[PARSESIZE];
	off_t	off;
	struct	direct  d[BSIZE/sizeof(struct direct)];
	register struct direct  *dp;
	int	l;

	/* lock directory - otherwise open/delete has hazard across rqsts */
	ilock(dir);
	/* increment usage so ifree works later */
	iuse(dir);
	DEBUGOUT(("file_stat\n"));

more:
	c = parsename(mp, fname);
	if(c == -1) {
		/* parse failed */
		ifree(dir);
		mp->m_ercode = E_EINVAL;
		return;
	}

	/* truncate name to filesystem supported length */
	fname[NAMESIZE] = '\0';

	/* a zero length name is legal */
	if(fname[0] == 0)
		goto found;

	if(itype(dir) != IFDIR) {
		mp->m_ercode = E_LOOKUP;
		return;
	}

	/*
	 *  look for softenters
	 *  be careful to unlock inode before leaving
	 *  if found and c=='\0' then mp->m_handle=t_DUP(softfile);return;
	 *  if found and c!='\0' then t_JUMP(softfile, mp, mp);
	 */
	{
		register struct	sfenter	*sf;
		sf = sf_lookup(dir->i_fsys->fs_mntd.next, fname, dir->i_number);
		if(sf != (struct sfenter *)0) {  	/* found a softenter */
			/* We unlock to prevent deadlocks, watch hazards */
			ifree(dir);
			t_RELAY(sf->sf_hndl, mp);
		}
	}

	/* search through actual directory */
	for(off = 0 ;
	    (l = iread(dir, (off_t)off, (off_t)sizeof(d), d)) != 0 ;
	    off += l) {
	      for(dp = &d[l/sizeof(struct direct)] ; --dp >= &d[0] ; ) {
		if((dp->d_ino != FREEINO && dp->d_ino != SOFTINO)
		  && strncmp(dp->d_name, fname, NAMESIZE) == 0) {
			/* free the use of the parent directory */
			ifree(dir);
			/* there is a hazard if the entry is removed here */
			if((dir = icache(dir->i_fsys, dp->d_ino)) != NULLINODE) {
				/* the opened inode is locked and usage>0 */
				if(c == '/') {
					if(itype(dir) == IFDIR)
						goto more;
					/* its not a directory */
					ifree(dir);
					mp->m_ercode = E_LOOKUP;
					return;
				}
found:
				if(mp->m_opcode == GETSTAT)
					mp->m_ercode = parsestat(mp, igstat, dir);
				else
					mp->m_ercode = parsestat(mp, ipstat, dir);
				ifree(dir);
				return;
			}
			mp->m_ercode = E_LOOKUP;
			return;
		}
	      }
	}

	/* no match */
	ifree(dir);
	mp->m_ercode = E_LOOKUP;
}


/*
 *  search the directory for the name designated by the message window and
 *    return a copy of the associated object
 *  the name is parsed until there is:
 *    none left
 *    no match (in which case you return an error)
 *    a nonlocal enter (in which case t_JUMP is used to continue the open)
 *  the directory inode should be unused
 */

drct_lookup(dir, mp)
register struct inode    *dir;
register struct message  *mp;
{
	char	c, fname[PARSESIZE];
	off_t	off;
	off_t	xoff;
	struct	direct  d[BSIZE/sizeof(struct direct)];
	register struct direct  *dp;
	int	l;

	/* lock directory - otherwise open/delete has hazard across rqsts */
	ilock(dir);
	/* increment usage so ifree works later */
	iuse(dir);
	DEBUGOUT(("drct_open\n"));

more:
	c = parsename(mp, fname);
	if(c == -1) {
		/* parse failed */
		ifree(dir);
		mp->m_ercode = E_EINVAL;
		return;
	}

	/* truncate name to filesystem supported length */
	fname[NAMESIZE] = '\0';

	/* a zero length name is legal */
	if(fname[0] == 0)
		goto found;

	/* look for softenters */
	{
		register struct	sfenter	*sf;
		sf = sf_lookup(dir->i_fsys->fs_mntd.next, fname, dir->i_number);
		if(sf != (struct sfenter *)0) {  	/* found a softenter */
			if(c == 0) {
				if(mp->m_param[0] == CONNECT_ASIS) {
					/* can return entered handle as is */
					mp->m_handle = t_DUP(sf->sf_hndl);
					mp->m_ercode = (OP_HANDLE|E_OK);
					ifree(dir);
					return;
				}
				else {
					/* CONNECT is not tail recursive */
					mp->m_opcode = CONNECT;
					t_REQUEST(sf->sf_hndl, mp);
					if(mp->m_ercode == E_ASIS) {
						mp->m_handle = t_DUP(sf->sf_hndl);
						mp->m_ercode = (OP_HANDLE|E_OK);
					}
					/*
					 *  you can leave the directory locked during
					 *    the connect because the connect message
					 *    CANNOT be going to this directory
					 *  (if it could it would have been folded)
					 */
					ifree(dir);
					return;
				}
			}
			else {
				/* We unlock to prevent deadlocks, watch hazards */
				ifree(dir);
				t_RELAY(sf->sf_hndl, mp);
			}
		}
	}

	/* search through actual directory */
	for(xoff = dir->i_size, off = 0 ;
	    (l = iread(dir, (off_t)off, (off_t)sizeof(d), d)) != 0 ;
	    off += l) {
	      for(dp = &d[l/sizeof(struct direct)] ; --dp >= &d[0] ; ) {
		if(dp->d_ino == FREEINO) {
			xoff = off + ( (int)dp - (int)d );
		}
		else if(strncmp(dp->d_name, fname, NAMESIZE) == 0) {
			/* free the use of the parent directory */
			ifree(dir);
			/* there is a hazard if the entry is removed here */
			if((dir = icache(dir->i_fsys, dp->d_ino)) != NULLINODE) {
				/* the opened inode is locked and usage>0 */
				if(c == '/') {
					if(itype(dir) == IFDIR)
						goto more;
					/* its not a directory */
					ifree(dir);
					mp->m_ercode = E_LOOKUP;
					return;
				}
found:
				if(mp->m_param[0] & CONNECT_TRUNC) {
					if(itype(dir) != IFREG) {
						/* only IFREG can be TRUNCed */
						ifree(dir);
						mp->m_ercode = E_LOOKUP;
						return;
					}
					else
						/* truncate regular file */
						itrunc(dir);
				}
				mp->m_handle = iopen(dir);
				if(mp->m_handle != NULLHNDL) {
					iunlock(dir);
					mp->m_ercode = E_OK | OP_HANDLE;
					return;
				}
				else
					iunlock(dir);
			}
			mp->m_ercode = E_LOOKUP;
			return;
		}
	      }
	}

	/* no match */
	if(c == '\0' && mp->m_param[0] & CONNECT_CREAT) {
		register struct inode    *ip;
debug("creat file %s in dir=%d (TYPE=%d)\n", fname, dir->i_number, mp->m_param[1]);
		/* the last component doesn't exist, creat it */
		switch(mp->m_param[1]) {
		    case PROT_FILE:
			ip = ialloc(dir->i_fsys, IFREG);
			break;
		    case PROT_DIRECT:
			ip = ialloc(dir->i_fsys, IFDIR);
			break;
		    default:
			ip = NULLINODE;
			break;
		}
		if(ip != NULLINODE) {
			mp->m_handle = iopen(ip);
			if(mp->m_handle != NULLHNDL) {
				ip->i_nlink++;
				ip->i_flag |= ICHG;
				dp = &d[0];
				dp->d_ino = ip->i_number;
				strncpy(dp->d_name, fname, NAMESIZE);
				if(iwrite(dir, (off_t)xoff, (off_t)sizeof(*dp), dp) !=
				   sizeof(*dp))
					panic("can't write directory entry\n");
				iunlock(ip);
				mp->m_ercode = E_OK|OP_HANDLE;
			}
			else
				mp->m_ercode = E_LOOKUP;
		}
		else
			mp->m_ercode = E_SPACE;
	}
	else
		mp->m_ercode = E_LOOKUP;
	ifree(dir);
}


/*
 *  drct_enter(inode, message, soft) - will and enter the passed object under the
 *	name passed in the data window.
 *	the name is parsed until there is:
 *	  none left
 *	  a nonlocal enter (in which case a t_JUMP continues the enter)
 *	  there is a complete match (in which case you return an error)
 *	If soft is non-zero softenter the item if a hard enter fails
 */

drct_enter(dir, mp, soft)
register struct inode    *dir;
register struct message  *mp;
{
	char	c, fname[PARSESIZE];
	off_t	off;
	off_t	xoff;
	struct	direct  d[BSIZE/sizeof(struct direct)];
	register struct direct  *dp;
	struct	inode	*inodep;
	int	(*hndlrp)();
	int	l;

	/* lock directory - otherwise enter/delete has hazard across rqsts */
	ilock(dir);
	/* increment usage so ifree works later */
	iuse(dir);
	DEBUGOUT(("drct_enter\n"));

more:
	c = parsename(mp, fname);
	if(c == -1) {
		/* parse failed */
		ifree(dir);
		t_CLOSE(mp->m_handle);
		mp->m_ercode = E_EINVAL;
		return;
	}

	/* truncate name to filesystem supported length */
	fname[NAMESIZE] = '\0';

	/* look for softenters */
	/* be careful to unlock inode before leaving */
	/* if found and c == '\0' then error */
	/* if found and c != '\0' then t_JUMP(softfile, mp, mp); */
	{
		register struct	sfenter	*sf;
		sf = sf_lookup(dir->i_fsys->fs_mntd.next, fname, dir->i_number);
		if(sf != (struct sfenter *)0) {
		  	/* found a softenter */
			if(c == 0) {
				t_CLOSE(mp->m_handle);
				mp->m_ercode = E_LOOKUP;
				ifree(dir);
				return;
			}
			else {
				/* We unlock to prevent deadlocks, watch hazards */
				ifree(dir);
				t_RELAY(sf->sf_hndl,mp);
			}
		}
	}

	/* search through actual directory */
	for(xoff = dir->i_size, off = 0 ;
	    (l = iread(dir, (off_t)off, (off_t)sizeof(d), d)) != 0 ;
	    off += l) {
	      for(dp = &d[l/sizeof(struct direct)] ; --dp >= &d[0] ; ) {
		if(dp->d_ino == FREEINO) {
			xoff = off + ( (int)dp - (int)d );
		}
		else if(strncmp(dp->d_name, fname, NAMESIZE) == 0) {
			/* there is a hazard if the entry is removed here */
			if(c == '/') {
				/* free the use of the parent directory */
				ifree(dir);
				dir = icache(dir->i_fsys, dp->d_ino);
				if(dir != NULLINODE) {
					if(itype(dir) == IFDIR)
						goto more;
					/* its not a directory */
					ifree(dir);
					t_CLOSE(mp->m_handle);
					mp->m_ercode = E_LOOKUP;
					return;
				}
			}
			DEBUGOUT(("enter failed (the name matched)\n"));
			if(!soft) {
				t_CLOSE(mp->m_handle);
				mp->m_ercode = E_LOOKUP;
				ifree(dir);
				return;
			}
			/* if the name matched and it is a softenter, do it anyway */
			if(t_FOLD(mp->m_handle, &hndlrp, &inodep) != E_OK ||
			   hndlrp != stat_handler &&
			   hndlrp != file_handler &&
			   hndlrp != drct_handler ||
			   inodep->i_fsys != dir->i_fsys) {
				/* it can't be folded -- soft enter it */
				register struct	sfenter	*sf;
				if((sf = sf_alloc()) == (struct sfenter *)0) {
					ifree(dir);
					t_CLOSE(mp->m_handle);
					mp->m_ercode = E_ENTER;
					return;
				}
				else {
					sf->sf_direct = dir->i_number;
					strcpy(sf->sf_name,fname);
					sf->sf_hndl = mp->m_handle;
					l_inlink(&dir->i_fsys->fs_mntd,	&sf->sf_next);
					mp->m_ercode = E_OK;
					printf("SOFTENTERED handle as %s\n", fname);
					ifree(dir);
					return;
				}
			}

			t_CLOSE(mp->m_handle);
			mp->m_ercode = E_LOOKUP;
			ifree(dir);
			return;
		}
	      }
	}

	if(c != '\0') {
		/* there are more name components */
		DEBUGOUT(("missing directory in enter\n"));
		ifree(dir);
		t_CLOSE(mp->m_handle);
		mp->m_ercode = E_LOOKUP;
		return;
	}

	if(t_FOLD(mp->m_handle, &hndlrp, &inodep) != E_OK ||
	   hndlrp != stat_handler &&
	   hndlrp != file_handler &&
	   hndlrp != drct_handler ||
	   inodep->i_fsys != dir->i_fsys) {
		if(soft) {
			register struct	sfenter	*sf;
			/* it can't be folded -- soft enter it */
			if((sf = sf_alloc()) == (struct sfenter *)0) {
				ifree(dir);
				t_CLOSE(mp->m_handle);
				mp->m_ercode = E_ENTER;
				return;
			}
			else {
				sf->sf_direct = dir->i_number;
				strcpy(sf->sf_name,fname);
				sf->sf_hndl = mp->m_handle;
				l_inlink(&dir->i_fsys->fs_mntd,&sf->sf_next);
				mp->m_ercode = E_OK;
				printf("SOFTENTERED handle as %s\n",fname);
				ifree(dir);
				return;
			}
		}

		DEBUGOUT(("can't fold handle\n"));
		ifree(dir);
		t_CLOSE(mp->m_handle);
		mp->m_ercode = E_ENTER;
		return;
	}

	/* hard enter the name in the saved slot */
	dp = &d[0];
	dp->d_ino = inodep->i_number;
	strncpy(dp->d_name, fname, NAMESIZE);
	if(iwrite(dir, (off_t)xoff, (off_t)sizeof(*dp), dp) != sizeof(*dp))
		panic("can't write directory entry");
	ifree(dir);
	/*
	 *  there may be a hazard here
	 *  don't worry about the usage of inodep since it will be resolved
	 *    by a CLOSE later (if nesessary)
	 */
	ilock(inodep);
	inodep->i_nlink++;
	inodep->i_flag |= ICHG;
	iunlock(inodep);
	t_CLOSE(mp->m_handle);

	mp->m_ercode = E_OK;
}


/*
 *  drct_delete(inode, message) - will delete the named object named
 */

drct_delete(dir, mp)
register struct inode    *dir;
register struct message  *mp;
{
	int	i;
	char	c, fname[PARSESIZE];
	off_t	off;
	struct	direct  d[BSIZE/sizeof(struct direct)];
	register struct direct  *dp;
	ino_t	ino;
	int	l;

	/* lock directory - otherwise enter/delete has hazard across rqsts */
	ilock(dir);
	/* increment usage so ifree works later */
	iuse(dir);
	DEBUGOUT(("drct_delete\n"));

more:
	c = parsename(mp, fname);
	if(c == -1) {
		/* parse failed */
		ifree(dir);
		mp->m_ercode = E_EINVAL;
		return;
	}

	/* truncate name to filesystem supported length */
	fname[NAMESIZE] = '\0';

	/* look for softenters */
	/* be careful to unlock inode before leaving */
	/* if found and c == '\0' then delete and t_CLOSE handle */
	/* if found and c != '\0' then t_JUMP(softfile, mp, mp); */
	{
		register struct	sfenter	*sf;
		sf = sf_lookup(dir->i_fsys->fs_mntd.next, fname, dir->i_number);
		if(sf != (struct sfenter *)0) {
		  	/* found a softenter */
			if(c == 0) {
				t_CLOSE(sf->sf_hndl);
				l_unlink(&sf->sf_next);
				sf_free(sf);
				mp->m_ercode = E_OK;
				printf("Deleted SOFTENTER %s\n",fname);
				ifree(dir);
				return;
			}
			else {
				/* We unlock to prevent deadlocks, watch hazards */
				ifree(dir);
				t_RELAY(sf->sf_hndl, mp);
			}
		}
	}

	/* search through actual directory */
	for(off = 0 ;
	    (l = iread(dir, (off_t)off, (off_t)sizeof(d), d)) != 0 ;
	    off += l) {
	      for(dp = &d[l/sizeof(struct direct)] ; --dp >= &d[0] ; ) {
		if(dp->d_ino != FREEINO &&
		   strncmp(dp->d_name, fname, NAMESIZE) == 0) {
			ino = dp->d_ino;
			if(c != '/') {
				dp->d_ino = FREEINO;
				if(iwrite(dir,
					  (off_t)(off+(int)dp-(int)d),
					  (off_t)sizeof(*dp),
					  dp) != sizeof(*dp))
						panic("can't write directory entry");
			}
			/* free the use of the parent directory */
			ifree(dir);
			/* there is a hazard if the entry is removed here */
			if((dir = icache(dir->i_fsys, ino)) != NULLINODE) {
				if(c == '/') {
					if(itype(dir) == IFDIR)
						goto more;
					/* its not a directory */
					ifree(dir);
					mp->m_ercode = E_LOOKUP;
					return;
				}
				dir->i_nlink--;
				dir->i_flag |= ICHG;
				ifree(dir);
				mp->m_ercode = E_OK;
				return;
			}
			printf("delete failed (couldnt get inode)\n");
			mp->m_ercode = E_ENTER;
			return;
		}
	      }
	}

	ifree(dir);
	DEBUGOUT(("delete failed (it wasnt there)\n"));
	mp->m_ercode = E_LOOKUP;
}
