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


extern	struct thread *syserror();
extern	struct thread *sysrequest();
extern	struct thread *sysreply();
extern	struct thread *sysrelay();
extern	struct thread *sysrecall();
extern	struct thread *sysfetch();
extern	struct thread *sysstore();
extern	struct thread *syssubdom();
extern	struct thread *sysspawn();
extern	struct thread *sysmakeport();
extern	struct thread *sysdup();
extern	struct thread *sysclose();
extern	struct thread *sysspl();
extern	struct thread *sysfold();
	
struct	thread	*(*traptab[])() =
{
	syserror,
	sysrequest,		/* callcode = 1  */
	sysreply,		/* callcode = 2  */
	sysrelay,		/* callcode = 3  */
	sysfetch,		/* callcode = 4  */
	sysstore,		/* callcode = 5  */
	syssubdom,		/* callcode = 6  */
	sysspawn,		/* callcode = 7  */
	sysmakeport,		/* callcode = 8  */
	sysdup,			/* callcode = 9  */
	sysfold,		/* callcode = 10 */
	sysclose,		/* callcode = 11 */
	sysrecall,		/* callcode = 12 */
	sysspl,			/* callcode = 13 */
};
