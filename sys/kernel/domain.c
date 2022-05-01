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
#include	"../h/args.h"

#include 	"../h/a.out.h"


domain_t  domains[DOMAINS];


d_print(dp)
domain_t  *dp;
{
	if(dp == NULLDOMAIN) {
		for(dp = &domains[0] ; dp < &domains[DOMAINS] ; dp++)
			d_print(dp);
		return;
	}

	debug("D(%x):  state = ", dp);
	if(dp->d_state & D_ALLOC) {
		debug("D_ALLOC");
		debug("  h=%x  s=%x  m=%x  p=%d\n",
		      dp->d_handle, dp->d_tdmap.m_seg,
		      dp->d_tdmap.m_addr, dp->d_priority);
	}
	else
		debug("D_FREE\n");
}


/* allocate a domain object */

domain_t  *
d_alloc()
{
	domain_t  *dp;

	for (dp = &domains[0] ; dp < &domains[DOMAINS] ; dp++) {
		if (dp->d_state != D_FREE)
			continue;
		dp->d_state = D_ALLOC;

		/* set up the primary subdomain */
		dp->d_subdom.d_domain = dp;
		l_list(&dp->d_subdom.d_hlist);
		l_list(&dp->d_subdom.d_plist);

		l_list(&dp->d_pdead);

		/* set up activity information */
		l_list(&dp->d_schdlst);
		l_list(&dp->d_access);

		/* allocate port for GO commands on domain object */
		dp->d_handle = p_alloc(&(dp->d_subdom), NULL, NULL, 0);
		if(dp->d_handle == NULLHNDL)
			panic("can't allocate handle for domain port");

		/* make sure owner of GO handle is the kernel */
		h_attach(dp->d_handle, D_SYSTEM);

		dp->d_tdmap.m_addr = USERBASE;

		/* allocate zero length segment (it grows as its written) */
		/* someday there will be code to initialize maps instead */
		if ((dp->d_tdmap.m_seg = s_alloc((off_t)0, 0)) == NULLSEGMENT)
			panic("can't allocate segment for domain");
		debug("d_alloc(%x)  ", dp);
		d_print(dp);
		return(dp);
	}
	ERROR(E_NDOMAIN);
	return(NULLDOMAIN);
}


/* create a new domain */

hndl_t
domain_make()
{
	domain_t  *dp;
	hndl_t    h;

	/*
	 * create a new domain
	 * return a handle to a kernel domain port which will handle messages
	 *   to the domain object
	 * the new domain has an empty address space and the returned handle
	 *   can be used for fiddling its map
	 * it has to be decided what the rules are for interlocking changing
	 *   the mappings while threads are executing in the domain
	 * the domain object contains an implicit port over which GO messages
	 *   on the domain are relayed to start a thread executing at the
	 *   domain's entry location
	 * this complicates the garbage collection problem since there
	 *   is an implicit port on the domain as long as the port on the
	 *   domain object is in existance.
	 */

	debug("MAKE_DOMAIN\n");
	/* allocate the domain */
	if((dp = d_alloc()) == NULLDOMAIN)
		panic("can't allocate domain");

	/*
	 * allocate a port with passport = domain pointer
	 * the handle comes back attached to D_SYSTEM
	 * the return from the resource handler will give it to the user
	 */
	h = p_alloc(&(D_SYSTEM->d_subdom), domain_handler, dp, 0);
	if (h == NULLHNDL)
		panic("can't allocate handle for domain object");
	return(h);
}


/*
 * free the domain and any resources it owns
 * note that this is normally called by the garbage collector since
 *   it is difficult to tell if the domain is really free
 */

d_free(dp)
domain_t *dp;
{
	/*
	 * the handle pointing to the domain is already free
	 *   either by the user or the GC
	 */
	ASSERT(dp->d_handle == NULLHNDL);

	/* free any handles that the domain owns */
	h_dfree(&(dp->d_subdom));

	/* free any segments the domain owns */
	/* this will be complicated by the addition of sharing with maps */
	s_free(dp->d_tdmap.m_seg);
	dp->d_state = D_FREE;
}


/*
 *  this is the handler for requests to manipulate the kernel domain
 *    object
 *  it is used in starting up a new domain and debugging an old one
 *  there will be some way to manipulate it in any way that an executing
 *    thread can
 */

void
domain_handler(dp, mp)
register struct	message	*mp;
register domain_t *dp;
{
	debug("domain_handler: PASSPORT=%X M_OPCODE=%X\n",
		dp, mp->m_opcode);
	d_print(dp);

	switch(mp->m_opcode) {
	    case CLOSE:
		debug("domain close\n");
		/* free (without a close) the handle used for GO */
		h_free(dp->d_handle);
		dp->d_handle = NULLHNDL;

		if(EMPTYLIST(dp->d_schdlst) && EMPTYLIST(dp->d_access) &&
		   EMPTYLIST(dp->d_subdom.d_plist)) {
			/*
			 *  no active threads through the domain and 
			 *  no ports accessing it
			 *    (this is the simple unix style gc situation)
			 */
			/* mark it as dead so no new threads get spawned */
			dp->d_state |= D_DEAD;

			d_free(dp);
		}
		break;

	    case GO:
		debug("domain go\n");
		/* forward GO into actual domain */
		debug("before sysrelay...\n");
		t_RELAY(dp->d_handle, mp);
		return;

	    case LOAD:
	    {
		off_t	size, doffs;
		struct	exec	hdr;

		debug("domain load\n");
		/* stat the file to check for executable mode */

		/* get the header */
		if((t_READ(mp->m_handle, 0, &hdr, sizeof(hdr)) !=
		    sizeof(hdr)) || N_BADMAG(hdr)) {
			mp->m_ercode = E_NOEXEC;
			return;
		}

		/* extend segment to correct size */
		doffs = hdr.a_text;
		if(hdr.a_magic != OMAGIC)
			/* round size of text up to page size */
			doffs = ctob(btoc(doffs));
		size = doffs + hdr.a_data + hdr.a_bss;
		if(size > (off_t)(STACKTOP-2*STACKSIZE))
			panic("exec in domain: %d too big\n", size);
		debug("[%d:%d:%d] new size is %d, doffs is %d\n",
			hdr.a_text, hdr.a_data, hdr.a_bss,
			size, doffs);
		if(s_expand(dp->d_tdmap.m_seg, size-dp->d_tdmap.m_seg->s_size)) {
			mp->m_ercode = E_SIZE;
			return;
		}

		/* force remap of domain */
		D_MAPPED = NULLSEGMENT;

		/*
		 * the problem here is that we need a way to refer to the
		 *   address space of the domain (for the read)
		 * we change the current state of the thread stack and change
		 *   it so its data window will point to the domain segment
		 * since LOAD doesn't use a data window this is safe
		 */
		T_CURRENT->t_dwmap.m_seg = dp->d_tdmap.m_seg;
		T_CURRENT->t_dwmap.m_addr = DWBASE;
		T_CURRENT->t_dwbase = DWBASE;
		T_CURRENT->t_dwbound = DWBASE + size;

		/* force remap of window */
		W_MAPPED = NULLSEGMENT;

		/* read from the file into the segment */
		if(doffs) {
			if(!read1k(mp->m_handle,
				   N_TXTOFF(hdr),
				   mp->m_dwbase,
				   hdr.a_text) ||
			   !read1k(mp->m_handle,
				   N_TXTOFF(hdr)+hdr.a_text,
				   mp->m_dwbase+doffs,
				   hdr.a_data) ) {
				mp->m_ercode = E_NOLOAD;
				return;
			}
		}
		else {
			if(!read1k(mp->m_handle,
				   sizeof(hdr),
				   mp->m_dwbase,
				   hdr.a_text+hdr.a_data)) {
				mp->m_ercode = E_NOLOAD;
				return;
			}
		}

		/* since we preread the whole segment close the handle */
		t_CLOSE(mp->m_handle);

		mp->m_ercode = E_OK;
		return;
	    }

	    case WRITE:
	    {
		off_t	 tdaddr;
		off_t	 tdsize;
		/*
		 * write into the address space of the domain
		 * the data gets written to the closest segment that starts
		 *   before the write address
		 * if the segment needs to be extended to support the write
		 *   address an attempt will be made to do so
		 * note that it is best to force a segment to the desired
		 *   length by doing a zero length write and then writing the
		 *   data rather than doing a number of short writes each of
		 *   which may require the segment be copied (to extend it)
		 */

		debug("domain write ");

		/* use offset as start of write */
		tdaddr = (caddr_t)mp->m_param[0] - dp->d_tdmap.m_addr;
		/* minimal length of domain */
		tdsize = tdaddr + mp->m_dwsize;

		debug("addr:%x size:%x ssz:%x\n",
			tdaddr, tdsize, dp->d_tdmap.m_seg->s_size);

		if(tdsize > (off_t)(STACKTOP-2*STACKSIZE))
			panic("write to domain: %d > 64000", tdsize);

		if(dp->d_tdmap.m_seg->s_size < tdsize) {
			/* must expand domain - expand it extra */
			tdsize += 5*PAGESIZE;
			debug("expanding domain size from %d to %d\n",
				dp->d_tdmap.m_seg->s_size, tdsize);
			if(s_expand(dp->d_tdmap.m_seg,
				    tdsize-dp->d_tdmap.m_seg->s_size)) {
				mp->m_ercode = E_SIZE;
				mp->m_param[0] = 0;
				return;
			}
			/* force remap of domain */
			D_MAPPED = NULLSEGMENT;
		}
		debug("copying data into domain\n");
		/* copy data out to segment */
		s_copy(mp->m_dwsize,
		       T_CURRENT->t_dwmap.m_seg,
			T_CURRENT->t_dwbase - T_CURRENT->t_dwmap.m_addr,
		       dp->d_tdmap.m_seg,
			tdaddr);
		mp->m_param[0] = mp->m_dwsize;
		debug("wrote new data into domain\n");
		d_print(dp);
		break;
	    }

	    case READ:
		/*
		 * read from the address space of the domain
		 * the data gets read from the closest segment whose start is
		 *   before the write address
		 */

		panic("domain read\n");

		if(dp->d_tdmap.m_seg->s_size < mp->m_param[0] + mp->m_dwsize) {
			mp->m_dwsize = dp->d_tdmap.m_seg->s_size - mp->m_param[0];
			if(mp->m_dwsize <= 0) {
				mp->m_param[0] = 0;
				mp->m_ercode = E_EOF;
				return;
			}
		}

		/* copy data in from domain */
		s_copy(mp->m_dwsize,
		       dp->d_tdmap.m_seg,
			mp->m_param[0],
		       T_CURRENT->t_dwmap.m_seg,
			T_CURRENT->t_dwbase - T_CURRENT->t_dwmap.m_addr);
		       
		mp->m_param[0] = mp->m_dwsize;
		break;

	    case ENTER:
	    {	char	name[PARSESIZE];
		int	c;

		/* enter a handle in the domain */
		
		debug("ENTER into domain\n");
		c = parsename(mp, name);
		if(c == 0) {
			if(strcmp(name, "RECALL") == 0) {
				printf("can't enter a recall handler\n");
/*				dp->d_recall = mp->m_handle;*/
				mp->m_ercode = E_OK;
				return;
			}
			/* unrecognized name */
		}
		mp->m_ercode = E_ENTER;
		return;
	    }

	    default:
		printf("domain unknown opcode\n");
		mp->m_ercode = E_OPCODE;
		return;
	}

	mp->m_ercode = E_OK;
}


/* do 1k byte reads for domain load (to support across net load for now) */

read1k(handle, off, buf, cnt)
hndl_t	handle;
{
	/* round up to 1k block */
	if((off & 1023) < cnt) {
		int	c = off & 1023;

		if(t_READ(handle, off, buf, c) != c)
			return(0);
		off += c;
		buf += c;
		cnt -= c;
	}

	/* read full 1k blocks */
	while(cnt >= 1024) {
		if(t_READ(handle, off, buf, 1024) != 1024)
			return(0);
		off += 1024;
		buf += 1024;
		cnt -= 1024;
	}

	/* read last partial block (if necessary) */
	if(cnt > 0) {
		if(t_READ(handle, off, buf, cnt) != cnt)
			return(0);
	}

	return(1);
}
