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

/*
 *  execute the program referenced by 'srchndl' with environment 'envhndl'
 *    create a new domain
 *    LOAD the program
 *    issue a t_GO to the domain with the data-window
 *	 (dwaddr = 'argb', dwsize = 'argl')
 */

t_EXEC(srchndl, argb, argl, envhndl)
hndl_t	srchndl;
char	*argb;
arg_t	argl;
hndl_t	envhndl;
{
	register ercode;
	struct message msg;
	register hndl_t	 domhndl;

	/* make a new domain	*/
	domhndl = t_LOOKUP(envhndl, "dev/resource/DOMAIN", CONNECT_ASIS);
	if(domhndl == NULLHNDL)
		return(E_NDOMAIN);

	/* load source program	*/
	if ((ercode = t_LOAD(domhndl, srchndl)) != E_OK)
		return(ercode);

	/* setup environment	*/
	t_DELETE(envhndl, "DOMAIN");
	t_ENTER(envhndl, "DOMAIN", t_DUP(domhndl));

	/* issue GO to domain	*/
	msg.m_opcode = GO;
	msg.m_handle = envhndl;
	msg.m_dwaddr = (arg_t)argb;
	msg.m_dwsize = argl;
	t_REQUEST(domhndl, &msg);
	t_CLOSE(domhndl);
	return((msg.m_param[0] << 16) | (msg.m_ercode));
}
