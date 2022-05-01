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
#include	"../h/link.h"
#include	"../h/assert.h"


/* initialize a link element */

l_init(link)
register struct	link	*link;
{
	FORASSERT(link->prev = NULLLINK);
	FORASSERT(link->next = NULLLINK);
}


/* initialize a list header */

l_list(list)
register struct	list	*list;
{
	list->prev = list->next = NULLLINK;
}


/* initialize the link by inserting it after the list element */

l_inlink(list, link)
register struct list	*list;
register struct	link	*link;
{
	/* link the link into a new list */
	if(list->next)
		list->next->prev = link;
	link->next = list->next;
	list->next = link;
	link->prev = (struct link *)list;
}


/* insert the link in the list */

l_link(list, link)
register struct list	*list;
register struct	link	*link;
{
	ASSERT(link->prev == NULLLINK && link->next == NULLLINK);
	/* link the link into a new list */
	if(list->next)
		list->next->prev = link;
	link->prev = (struct link *)list;
	link->next = list->next;
	list->next = link;
}


/* take the link out of the list */

l_unlink(link)
register struct link	*link;
{
	ASSERT(link->prev != NULLLINK && link->prev->next != NULLLINK);
	/* unlink the link from its current list */
	link->prev->next = link->next;
	if(link->next != NULLLINK)
		link->next->prev = link->prev;
	FORASSERT(link->prev = NULLLINK);
	FORASSERT(link->next = NULLLINK);
}


/* take the link out of its current list and insert it in a new list */

l_relink(list, link)
register struct list	*list;
register struct	link	*link;
{
	ASSERT(link->prev != NULLLINK && link->prev->next != NULLLINK);
	/* unlink the link from its current list */
	link->prev->next = link->next;
	if(link->next != NULLLINK)
		link->next->prev = link->prev;
	/* link the link into a new list */
	if(list->next)
		list->next->prev = link;
	link->prev = (struct link *)list;
	link->next = list->next;
	list->next = link;
}


/* return the first link in the given list */

struct	link	*
l_first(list)
register struct list	*list;
{
	return(list->next);
}


/* return the number of links in the list */

l_length(list)
register struct	list	*list;
{
	register struct	link	*l;
	register int	count = 0;

	FOREACHLINK(l, (*list))
		count++;
	return(count);
}
