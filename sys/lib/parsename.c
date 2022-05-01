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


parsename(mp, b)
register struct message	*mp;
register char	*b;
{
	register char	*f, *t;
		 int	term = 0;
		 int	cnt;

	/*
	 *  get the next element of a compound name
	 *  in the process:
	 *    remove '/' prefixes
	 *    parse only up to '\0', '/', or the end of the data window
	 */
	if(mp->m_dwsize == 0) {
		/* null name -- this avoids FETCH error */
		b[0] = 0;
		return(0);
	}

	if((cnt = mp->m_dwsize) > PARSESIZE - 1)
		cnt = PARSESIZE - 1;
	if((cnt = t_FETCH(mp->m_dwaddr, b, cnt)) < 0) {
		b[0] = 0;
		printf("can't parse name, fetch failed\n");
		return(-1);
	}

	/* add null to force termination of name */
	b[cnt] = 0;

	/* strip off / prefixes */
	for(f = b ; *f == '/' ; f++);

	/* find the terminator -- note that we NEVER parse past a NUL */
	for(t = b ; *f ; )
		if((*t++ = *f++) == '/') {
			t--;
			term = '/';
			break;
		}

	/* if the name was >= PARSESIZE you can't be sure its ok */
	if(f == &b[PARSESIZE-1]) {
		printf("can't parse name '%s'\n", b);
		b[0] = 0;
		return(-1);
	}
	/* otherwise, terminate it with a NUL */
	*t = 0;

	/* update the data window to reflect the part thats been parsed */
	mp->m_dwaddr += (f-b);
	mp->m_dwsize -= (f-b);

	DEBUGOUT(("  parsed '%s' %d\n", b, term));

	return(term);
}
