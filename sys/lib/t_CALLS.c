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


/************************************************************************

	TRIX 1.0
	Kernel Calls

	For the time being, most of these calls are coded in C.  The
	exceptions are t_REQUEST, t_REPLY, and t_RELAY, which are in
	assembly-language for efficiency.   All other  calls trap to
	the kernel via the general assembly-langauge interface provi-
	ded by t_CALL.

 ************************************************************************/


#define	MSGSIZE	((sizeof(struct message))/(sizeof(arg_t)))
#define DWSIZE	((sizeof(struct dwindow))/(sizeof(arg_t)))

extern	arg_t	t_CALL();

/*
t_REQUEST(target, mp)
hndl_t	target;
struct	message *mp;
{
	register i;
	register arg_t	*ap, *dp;
	arg_t	 args[N_ARGS];

	args[KERCALL] = K_REQUEST;
	args[TARGET] = (arg_t)target;
	ap = &args[M_OPCODE];
	dp = (arg_t *)mp;
	for (i = 0; i < MSGSIZE; i++)
		*ap++ = *dp++;
	t_CALL(args);
	ap = (arg_t *)mp;
	dp = &args[M_OPCODE];
	for (i = 0; i < MSGSIZE; i++)
		*ap++ = *dp++;
	return(rp->m_opcode);
}

t_REPLY(mp)
struct	message	*mp;
{
	register i;
	register arg_t	*ap, *dp;
	arg_t	args[N_ARGS];

	args[KERCALL] = K_REPLY;
	ap = &args[M_OPCODE];
	dp = (arg_t *)mp;
	for (i = 0; i < MSGSIZE; i++)
		*ap++ = *dp++;
	t_CALL(args);
}

t_RELAY(target, mp)
hndl_t	target;
struct	message	*mp;
{
	register i;
	register arg_t	*ap, *dp;
	arg_t	 args[N_ARGS];

	args[KERCALL] = K_RELAY;
	args[TARGET] = (arg_t)target;
	ap = &args[M_OPCODE];
	dp = (arg_t *)mp;
	for (i = 0; i < MSGSIZE; i++)
		*ap++ = *dp++;
	t_CALL(args);
}
 */


t_SPAWN(entry, passport, priority)
int	(*entry)();
{
	arg_t	args[N_ARGS];

	args[KERCALL] = K_SPAWN;
	args[M_OPCODE] = 0;
	args[PORT_ENTRY] = (arg_t)entry;
	args[PORT_PASSPORT] = passport;
	args[PORT_PRIORITY] = priority;
	t_CALL(args);
	return(args[R_OPCODE]);
}

t_SPL(priority)
{
	arg_t	args[N_ARGS];

	args[KERCALL] = K_SPL;
	args[M_OPCODE] = 0;
	args[THREAD_PRIORITY] = priority;
	t_CALL(args);
	return(args[R_PARAM0]);
}

t_CLOSE(h)
hndl_t	h;
{
	arg_t	args[N_ARGS];

	args[KERCALL] = K_CLOSE;
	args[M_OPCODE] = OP_HANDLE;
	args[M_HANDLE] = (arg_t)h;
	t_CALL(args);
	return(args[R_OPCODE]);
}

hndl_t
t_DUP(h)
hndl_t	h;
{
	arg_t	args[N_ARGS];

	args[KERCALL] = K_DUP;
	args[M_OPCODE] = OP_HANDLE;
	args[M_HANDLE] = (arg_t)h;
	t_CALL(args);
	if (args[R_OPCODE] & OP_HANDLE)
		return((hndl_t)args[R_HANDLE]);
	else
		return(NULLHNDL);
}

hndl_t
t_MAKEPORT(entry, passport, priority, subdom)
int	(*entry)();
{
	arg_t	args[N_ARGS];

	args[KERCALL] = K_MAKEPORT;
	args[M_OPCODE] = 0;
	args[PORT_ENTRY] = (arg_t)entry;
	args[PORT_PASSPORT] = passport;
	args[PORT_PRIORITY] = priority;
	t_CALL(args);
	if (args[R_OPCODE] & OP_HANDLE)
		return((hndl_t)args[R_HANDLE]);
	else
		return(NULLHNDL);
}


t_FETCH(offset, buffer, nbytes)
{
	arg_t	args[N_ARGS];

#ifdef KERNEL
	if(kkernel())
		return(kfetch(offset, buffer, nbytes));
#endif

	args[KERCALL] = K_FETCH;
	args[M_OPCODE] = DW_WRITE;
	args[M_DWADDR] = buffer;
	args[M_DWSIZE] = nbytes;
	args[M_PARAM0] = offset;
	t_CALL(args);
	return(args[R_PARAM0]);
}

t_STORE(offset, buffer, nbytes)
{
	arg_t	args[N_ARGS];

#ifdef KERNEL
	if(kkernel())
		return(kstore(offset, buffer, nbytes));
#endif

	args[KERCALL] = K_STORE;
	args[M_OPCODE] = DW_READ;
	args[M_DWADDR] = buffer;
	args[M_DWSIZE] = nbytes;
	args[M_PARAM0] = offset;
	t_CALL(args);
	return(args[R_PARAM0]);
}


t_FOLD(h, fp, dp)
hndl_t	h;
arg_t   *fp, *dp;
{
	arg_t	args[N_ARGS];

	args[KERCALL] = K_FOLD;
	args[M_OPCODE] = OP_HANDLE;
	args[M_HANDLE] = (arg_t)h;
	t_CALL(args);
	if (args[R_OPCODE] == E_OK)
	{
		if (fp != (arg_t *)NULL)
			*fp = args[PORT_ENTRY];
		if (dp != (arg_t *)NULL)
			*dp = args[PORT_PASSPORT];
	}
	return(args[R_OPCODE]);
}


t_RECALL(reason)
arg_t	reason;
{
	arg_t	args[N_ARGS];

	args[KERCALL] = K_RECALL;
	args[M_OPCODE] = 0;
	args[M_PARAM0] = reason;
	t_CALL(args);
	return(args[R_OPCODE]);
}

