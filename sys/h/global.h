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


hndl_t		p_alloc();
hndl_t		h_alloc();

hndl_t		h_hndl();
handle_t	*h_point();
port_t		*h_port();


domain_t	*d_alloc();
void		domain_handler();

domain_t	*D_CURRENT;		/* the current domain (for spl) */
domain_t	*D_SYSTEM;		/* the "system" domain */


thread_t	*schedule();
int		synchro;
int		sync_handler();


void		env_handler();

thread_t	*t_spawn();
thread_t	*t_signal();
thread_t	*t_push();
thread_t	*t_pop();


struct	link	*l_first();


segment_t	*s_alloc();
thread_t	*T_CURRENT;		/* the currently executing thread */
segment_t	*D_MAPPED;		/* the currently mapped domain */
segment_t	*T_MAPPED;		/* the currently mapped thread */
segment_t	*W_MAPPED;		/* the currently mapped window */
int		flushcache;		/* force a cache flush at next memmap */
char		*maxcore;		/* pointer to last phys address */
long		stacksize;		/* size of initial stack */

int		Debug;			/* flag that controls debug output */

int		dosched;		/* flag used to force scheduling */
int		insched;		/* flag used to account for idle */
int		system();		/* the function executed by T0 */

int		cpu_type;		/* 68000 or 68010 for now */
