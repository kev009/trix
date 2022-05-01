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


struct	list {
	struct	link	*next;
	struct	link	*prev;
};


struct	link {
	struct	link	*next;
	struct	link	*prev;
};


/* get the entire structure back from its link pointer */
#define	LINKREF(l,s,t)	((t)((char *)(l) - (int)(&((t)0)->s)))


/* loop over links in a list, entries cannot be removed during loop */
#define	FOREACHLINK(l,ls)	for(l=ls.next ; l!=NULLLINK ; l=l->next)

/* loop over entries in a list, entries can be removed during loop */
#define	FOREACHENTRY(l,ls,s,e,t)	\
				for(l=ls.next ; l!=NULLLINK && \
						((e=LINKREF(l,s,t)) || 1) && \
						((l = l->next) || 1) ; )


/* return the first link in the given list */
#define	L_FIRST(list)		((list)->next)

/* return next link */
#define	L_NEXT(link)		((link)->next)

/* is there anything in this list */
#define	EMPTYLIST(ls)		(ls.next == NULLLINK)


/* initialize a link element */

#define	L_INIT(link)							\
{									\
	FORASSERT(link->prev = NULLLINK);				\
	FORASSERT(link->next = NULLLINK);				\
}


/* initialize a list header */

#define	L_LIST(list)							\
{									\
	list->prev = list->next = NULLLINK;				\
}


/* initialize the link by inserting it after the list element */

#define	L_INLINK(list, link)						\
{									\
	link->next = list->next;					\
	list->next = link;						\
	link->prev = (struct link *)list; 				\
}


/* insert the link in the list */

#define	L_LINK(list, link)						\
{									\
	ASSERT(link->prev == NULLLINK && link->next == NULLLINK);	\
	/* link the link into a new list */				\
	link->prev = (struct link *)list;				\
	link->next = list->next;					\
	list->next = link;						\
}


/* take the link out of the list */

#define	L_UNLINK(link)							\
{									\
	ASSERT(link->prev != NULLLINK && link->prev->next != NULLLINK);	\
	/* unlink the link from its current list */			\
	link->prev->next = link->next;					\
	if(link->next != NULLLINK)					\
		link->next->prev = link->prev;				\
	FORASSERT(link->prev = NULLLINK);				\
	FORASSERT(link->next = NULLLINK);				\
}


/* take the link out of its current list and insert it in a new list */

#define	L_RELINK(list, link)						\
{									\
	ASSERT(link->prev != NULLLINK && link->prev->next != NULLLINK);	\
	/* unlink the link from its current list */			\
	link->prev->next = link->next;					\
	if(link->next != NULLLINK)					\
		link->next->prev = link->prev;				\
	/* link the link into a new list */				\
	link->prev = (struct link *)list;				\
	link->next = list->next;					\
	list->next = link;						\
}
