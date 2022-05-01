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
#include "../h/status.h"
#include "../h/assert.h"


/*
 *  the data window for GETSTAT/PUTSTAT are of the form:
 *    filename\0
 *    { data_size[1], opclass[1], opcode[2], data_space[data_size-3] }*
 *  where data_size is the size of the rest of the field (not including data_size)
 *    (thus nulls are skipped)
 */


#define	DATA(s)		(&s[4])
#define	SIZE(s)		(s[0] & 0177)
#define	BUMP(s)		(s += SIZE(s))
#define	TYPE(s)		((s[0]) | (s[1]<<8) | (s[2]<<16) | (s[3]<<24))

#define	STATSIZE	256


kparsestat(mp, process, passport)
register struct message	*mp;
long	(*process)();
char	*passport;
{
	unsigned char	  statbuf[STATSIZE+1];
	register unsigned char	*s = statbuf;
	register long	 val;
	int	 c;

	if(mp->m_dwsize > STATSIZE)
		return(E_TOOBIG);
	c = kfetch(mp->m_dwaddr, statbuf, STATSIZE);

	while(s < &statbuf[c]) {
		if(*s) {
			ASSERT(SIZE(s) == sizeof(long)+4);
			if(&s[SIZE(s)] > &statbuf[c])
				return(E_STATPARSE);
			val = STATLONG(DATA(s));
			val = (*process)(passport, TYPE(s), val);
			LONGSTAT(DATA(s), val);
			BUMP(s);
		}
		else
			s++;
	}

	kstore(mp->m_dwaddr, statbuf, c);
	return(E_OK);
}


simplestat(mp, getf, putf, passport)
register struct message	*mp;
long	(*getf)();
long	(*putf)();
char	*passport;
{
	char	name[PARSESIZE];
	char	c;

	c = kparsename(mp, name);
	if(c == -1)
		return(E_EINVAL);

	/* null name -- return the status of the clock hndl */
	if(c == 0 && name[0] == '\0') {
		if(mp->m_opcode == GETSTAT)
			return(kparsestat(mp, getf, passport));
		else
			return(kparsestat(mp, putf, passport));
	}
	return(E_LOOKUP);
}
