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
#include "../h/unixtypes.h"
#include	"param.h"
#include	"fsys.h"
#include	"inode.h"


/* init soft enter freelist */

sf_init()
{
	int	i;	

	l_list(&sffree);
	for(i = 0 ; i < SFENTER ; i++)
		l_inlink(&sffree, &sfenter[i].sf_next);
}


struct sfenter *
sf_alloc()
{
	struct	link	*l;
	struct	sfenter	*sf;

	/* The operation must be atomic with respect to thread scheduling */
	if((l = sffree.next) == NULLLINK) {
		printf("Out of softenter structures, sorry Steve\n");
		return((struct sfenter *)0);
	}
	l_unlink(l);
	sf = LINKREF(l, sf_next, struct sfenter *);
	sf->sf_name[0] = 0;
	sf->sf_direct = FREEINO;
	sf->sf_hndl = NULLHNDL;
	sf->sf_next.next = NULLLINK;
	return(sf);
}


sf_free(sf)
struct	sfenter	*sf;
{
	l_inlink(&sffree, &sf->sf_next);
}


struct	sfenter	*
sf_lookup(l, name, ino)
struct	link	*l;
char	*name;
ino_t	ino;
{
	struct	sfenter	*sf;

	while(l != NULLLINK) {
		sf = LINKREF(l, sf_next, struct sfenter *);
		/* Ought to check bounds on sf for validity */
		if(strcmp(sf->sf_name,name) == 0)
			if((ino == 0) || (sf->sf_direct == ino))
				return(sf);
		l = l->next;
	}
	return((struct sfenter *)0);
}
