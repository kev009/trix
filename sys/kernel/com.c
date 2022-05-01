#include	"../h/param.h"
#include	"../h/calls.h"
#include	"../h/global.h"
#include	"../h/link.h"
#include	"../h/com.h"
#include	"../h/memory.h"
#include	"../h/domain.h"
#include	"../h/thread.h"
#include	"../h/assert.h"


/* the port table */
port_t    ports[PORTS];

/* the handle table */
handle_t  handles[HANDLES];

/* the list of kernel (P_KERNEL) ports */
struct	list	kernelports;


/* initialize the ports table */

p_init()
{
}


/* allocate a normal (non kernel) port and return a referencing handle */

hndl_t
p_alloc(subdom, entry, passport, priority)
subdom_t  *subdom;
int	  (*entry)();
arg_t	  passport;
{
	register hndl_t  h;
	register port_t  *pp;

	for(pp = ports ; pp < &ports[PORTS] ; pp++)
		if(pp->p_state == P_FREE) {
			/* mark port allocated */
			pp->p_state = P_USER;

			/* link this port into the creating domain's active list */
			l_link(&subdom->d_plist, &pp->p_link);

			/* allocate a handle to it */
			if((h = h_alloc(pp, subdom)) == NULLHNDL)
				break;

			/* initialize port with corrent info */
			pp->p_subdom = subdom;
			pp->p_entry = entry;
			pp->p_passport = passport;
			pp->p_priority = priority;

			return(h);
		}
	ERROR(E_NPORT);
	return(NULLHNDL);
}


/* allocate a special (kernel) port and return a referencing handle */

hndl_t
p_kalloc(entry, passport, priority)
int	  (*entry)();
arg_t	  passport;
{
	register hndl_t  h;
	register port_t  *pp;

	for(pp = ports ; pp < &ports[PORTS] ; pp++)
		if(pp->p_state == P_FREE) {
			/* mark port allocated */
			pp->p_state = P_KERNEL;

			/* link this port into the kernel port active list */
			l_link(&kernelports, &pp->p_link);

			/* allocate a handle to it */
			if((h = h_alloc(pp, &(D_SYSTEM->d_subdom))) == NULLHNDL)
				break;

			/* initialize port with corrent info */
			pp->p_subdom = &(D_SYSTEM->d_subdom);
			pp->p_entry = entry;
			pp->p_passport = passport;
			pp->p_priority = priority;

			return(h);
		}
	ERROR(E_NPORT);
	return(NULLHNDL);
}
	

/* free the specified port without a close message (used by garbage collector) */

p_free(pp)
register port_t *pp;
{
	/* remove port from the active list */
	l_unlink(&pp->p_link);

	/* actually free the port */
	pp->p_state = P_FREE;
}


/*
 *  free the specified port (spawning a close message)
 *  called both by h_close and THE close thread doing a final reply
 */

p_close(pp)
register port_t *pp;
{
	/* kernel ports are treated specially */
	if(pp->p_state & P_KERNEL) {
		struct	message	m;

		/* at this point we are on the kernel stack, or does it matter? */
		/* call directly into the handler */
		m.m_opcode = CLOSE;
/*		pp->p_priority = priority;	*/
		if((*pp->p_entry)(pp->p_passport, &m)) {
			/* close requires rerun */
			panic("INCOMPLETE KERNEL CLOSE\n");
		}

		/* remove port from the active list */
		l_unlink(&pp->p_link);

		/* free the actual port */
		pp->p_state = P_FREE;
		return;
	}

	/* user ports require (eventually) spawning a thread to send close message */
	if(pp->p_subdom->d_domain->d_state & D_DEAD) {
		/* if the domain is dead don't do close messages */
		debug("closing port on dead domain\n");

		/* remove port from the active list */
		l_unlink(&pp->p_link);

		/* actually free the port */
		pp->p_state = P_FREE;
	}
	else if((pp->p_subdom->d_domain->d_state & D_CLOSE) == 0) {
		/* there is not already a close thread */
		debug("SPAWNING CLOSING THREAD\n");

		/* note that a close thread is now running in the domain */
		pp->p_subdom->d_domain->d_state |= D_CLOSE;

		/* spawn the close thread */
		t_spawn(NULLTHREAD,
			&pp->p_subdom->d_domain->d_subdom,
			pp->p_entry,
			pp->p_passport,
			pp->p_priority,
			CLOSE);

		/* remove port from the active list */
		l_unlink(&pp->p_link);

		/* actually free the port */
		pp->p_state = P_FREE;
	}
	else {
		/* there is a close thread -- put it off for a while */
		debug("waiting on close of %x\n", pp);

		/* move port to domain close list */
		l_relink(&pp->p_subdom->d_domain->d_pdead, &pp->p_link);

		/* mark port as having been closed */
		pp->p_state = P_CLOSED;
		/* the threads reply will p_free it again later */
	}
}


/* initialize the handle table */

h_init()
{
}


/* allocate a handle to port */

hndl_t
h_alloc(pp, dp)
register port_t *pp;
register subdom_t  *dp;
{
	register handle_t *hp;

	for(hp = &handles[1] ; hp < &handles[HANDLES] ; hp++)
		if(hp->h_port == NULLPORT) {
			/* increment handle count on port */
			pp->p_handles++;

			/* allocate it to the specified port */
			hp->h_port = pp;
			hp->h_subdom = dp;

			/* "convert" hp to hkname (NOP) */
			return(h_hndl(hp));
		}
	ERROR(E_NHANDLE);
	return(NULLHNDL);
}


/ª attach a handle to a subdom */

h_attach(h, dp)
register hndl_t   h;
register subdom_t *dp;
{
	if(h != NULLHNDL) {
		debug("h_attach(%d,%x)\n", h, dp);
		h_point(h)->h_subdom = dp;
	}
	else
		printf("h_attach(NULLHNDL)\n");
}


/* free the specified handle, on last reference to a port, free the port also */

h_free(h)
register hndl_t  h;
{
	register port_t *pp;

	pp = h_point(h)->h_port;

	/* free the handle */
	h_point(h)->h_subdom = NULLSUBDOM;
	h_point(h)->h_port = NULLPORT;

	if(--pp->p_handles == 0)
		p_free(pp);
}


/* free the handle, on last reference to a port, CLOSE the port */

h_close(h)
register hndl_t	h;
{
	register port_t *pp;

	pp = h_point(h)->h_port;
	/* free the handle */
	h_point(h)->h_subdom = NULLSUBDOM;
	h_point(h)->h_port = NULLPORT;

	if(--pp->p_handles == 0)
		p_close(pp);
}


/* free all the handles on the specified subdom */

h_dfree(dp)
register subdom_t *dp;
{
	register handle_t *hp;

	for(hp = &handles[0] ; hp < &handles[HANDLES] ; hp++)
		if(hp->h_port != NULLPORT && hp->h_subdom == dp)
			h_close(h_hndl(hp));
}


/* return the port the handle points to */

port_t *
h_port(h)
register hndl_t  h;
{
	return(h_point(h)->h_port);
}


/*
 *  verify the pointer is legal kname belonging to the current subdom
 *    (usually called in ASSERTion)
 */

h_legal(h)
hndl_t  h;
{
	if(SYSSUBDOM(T_CURRENT->t_subdom)) {
		/* we have a higher standard for the system */
		if(!(h >= 1 && h < HANDLES && h_point(h))) {
			printf("h = %d  h->p = %x\n", h, h_point(h));
			prtrace(&(((int *)&h)[-2]));
			return(0);
		}
		ASSERT(h >= 1 && h < HANDLES && h_point(h));
		ASSERT(SYSSUBDOM(h_point(h)->h_subdom));
		/* you made it so it all must be ok */
		return(1);
	}

	if(h < 1 || h >= HANDLES) {
		printf("      ERROR handle %d out of range\n", h);
		return(0);
	}

	if(h_point(h)->h_subdom != T_CURRENT->t_subdom) {
		printf("      ERROR handle %d owned by domain %x\n",
			 h, h_point(h)->h_subdom);
		return(0);
	}

	return(1);
}


/* map a handle kname to a pointer to the structure */

handle_t *
h_point(h)
hndl_t  h;
{
	return(&handles[h]);
}


/* map a pointer to a hndl index */

hndl_t
h_hndl(hp)
handle_t  *hp;
{
	return(hp - handles);
}
