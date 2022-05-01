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
 *	TRIX 1.0
 *	Request Calls
 */


#define	E_OKH	(OP_HANDLE|E_OK)
#define E_RERR	(OP_FERROR)


hndl_t
t_CONNECT(tgthndl, access, proto)
register hndl_t	tgthndl;
{
	struct	message msg;
	int	ret;

	msg.m_opcode = CONNECT;
	msg.m_param[0] = access;
	msg.m_param[1] = proto;		/* this argument is optional */
	if((ret = t_REQUEST(tgthndl, &msg)) == E_OKH)
		return(msg.m_handle);
	/* NOTE: this is NOT tail recursive */
	if(ret == E_ASIS)
		return(t_DUP(msg.m_handle));
	return(NULLHNDL);
}

hndl_t
t_LOOKUP(tgthndl, name, connect, proto)
register hndl_t	tgthndl;
register char	*name;
{
	struct	message msg;
	int	ret;

	msg.m_opcode = LOOKUP;
	msg.m_dwaddr = name;
	msg.m_dwsize = strlen(name);
	msg.m_param[0] = connect;
	msg.m_param[1] = proto;		/* this argument is optional */
	if((ret = t_REQUEST(tgthndl, &msg)) == E_OKH)
		return(msg.m_handle);
	if(ret == E_ASIS)
		return(t_DUP(msg.m_handle));
	return(NULLHNDL);
}


t_ENTER(tgthndl, name, msghndl)
hndl_t	tgthndl;
char	*name;
hndl_t	msghndl;
{
	struct	message msg;

	msg.m_opcode = ENTER;
	msg.m_handle = msghndl;
	msg.m_dwaddr = name;
	msg.m_dwsize = strlen(name);
	t_REQUEST(tgthndl, &msg);
	return(msg.m_ercode);
}

t_SOFTENTER(tgthndl, name, msghndl)
hndl_t	tgthndl;
char	*name;
hndl_t	msghndl;
{
	struct	message msg;

	msg.m_opcode = SOFTENTER;
	msg.m_handle = msghndl;
	msg.m_dwaddr = name;
	msg.m_dwsize = strlen(name);
	t_REQUEST(tgthndl, &msg);
	return(msg.m_ercode);
}


t_DELETE(tgthndl, name)
hndl_t	tgthndl;
char	*name;
{
	struct	message msg;

	msg.m_opcode = DELETE;
	msg.m_dwaddr = name;
	msg.m_dwsize = strlen(name);
	if(t_REQUEST(tgthndl, &msg) != E_OK && msg.m_ercode != E_LOOKUP)
		msg.m_ercode |= E_RERR;
	return(msg.m_ercode);
}


t_GETSTAT(tgthndl, buf, cnt)
hndl_t	tgthndl;
char	*buf;
{
	struct	message	msg;

	msg.m_opcode = GETSTAT;
	msg.m_dwaddr = buf;
	msg.m_dwsize = (off_t)cnt;
	t_REQUEST(tgthndl, &msg);
	return(msg.m_ercode);
}

t_PUTSTAT(tgthndl, buf, cnt)
hndl_t	tgthndl;
char	*buf;
{
	struct	message	msg;

	msg.m_opcode = PUTSTAT;
	msg.m_dwaddr = buf;
	msg.m_dwsize = (off_t)cnt;
	t_REQUEST(tgthndl, &msg);
	return(msg.m_ercode);
}


t_READ(tgthndl, off, buf, cnt)
hndl_t	tgthndl;
char	*buf;
{
	struct	message msg;

	msg.m_opcode = READ;
	msg.m_dwaddr = buf;
	msg.m_dwsize = (off_t)cnt;
	msg.m_param[0] = (arg_t)off;
	msg.m_param[1] = (arg_t)-1;	/* no readahead */
	if(t_REQUEST(tgthndl, &msg) == E_OK)
		return(msg.m_param[0]);
	return(msg.m_ercode|E_RERR);
}

t_READA(tgthndl, off, buf, cnt, preseek)
hndl_t	tgthndl;
char	*buf;
{
	struct	message msg;

	msg.m_opcode = READ;
	msg.m_dwaddr = buf;
	msg.m_dwsize = (off_t)cnt;
	msg.m_param[0] = (arg_t)off;
	msg.m_param[1] = (arg_t)preseek; /* potential readahead location */
	if(t_REQUEST(tgthndl, &msg) == E_OK)
		return(msg.m_param[0]);
	return(msg.m_ercode|E_RERR);
}


t_WRITE(tgthndl, off, buf, cnt)
hndl_t	tgthndl;
char	*buf;
{
	struct	message msg;

	msg.m_opcode = WRITE;
	msg.m_dwaddr = buf;
	msg.m_dwsize = (off_t)cnt;
	msg.m_param[0] = (arg_t)off;
	if(t_REQUEST(tgthndl, &msg) == E_OK)
		return(msg.m_param[0]);
	return(msg.m_ercode|E_RERR);
}


t_LOAD(tgthndl, msghndl)
hndl_t	tgthndl;
hndl_t	msghndl;
{
	struct	message msg;

	msg.m_opcode = LOAD;
	msg.m_handle = msghndl;
	t_REQUEST(tgthndl, &msg);
	return(msg.m_ercode);
}

t_GO(tgthndl, msghndl, argbuf, arglen)
hndl_t	tgthndl;
hndl_t	msghndl;
char	*argbuf;
{
	struct	message msg;

	msg.m_opcode = GO;
	msg.m_handle = msghndl;
	msg.m_dwaddr = argbuf;
	msg.m_dwsize = (off_t)arglen;
	t_REQUEST(tgthndl, &msg);
	return(msg.m_ercode);
}


t_SLEEP(tgthndl, addr)
hndl_t	tgthndl;
{
	struct	message msg;

	msg.m_opcode = SLEEP;
	msg.m_param[0] = (arg_t)addr;
	t_REQUEST(tgthndl, &msg);
	return(msg.m_ercode);
}

t_WAKEUP(tgthndl, addr)
hndl_t	tgthndl;
{
	struct	message msg;

	msg.m_opcode = WAKEUP;
	msg.m_param[0] = (arg_t)addr;
	t_REQUEST(tgthndl, &msg);
	return(msg.m_ercode);
}


t_UPDATE(tgthndl)
hndl_t	tgthndl;
{
	struct	message	msg;

	msg.m_opcode = UPDATE;
	t_REQUEST(tgthndl, &msg);
	return(msg.m_ercode);
}


hndl_t
t_MOUNT(tgthndl, msghndl)
hndl_t	tgthndl;
hndl_t	msghndl;
{
	struct	message	msg;

	msg.m_opcode = MOUNT;
	msg.m_handle = msghndl;
	if(t_REQUEST(tgthndl, &msg) == E_OKH)
		return(msg.m_handle);
	return(NULLHNDL);
}
