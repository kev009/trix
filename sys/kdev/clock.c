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
#include	"../h/calls.h"
#include	"../h/link.h"
#include	"../h/status.h"
#include	"../h/protocol.h"
#include	<sys/timeb.h>


/* System-Clock Device Driver Routines */


#define	HZ	50		/* clock ticks per second	*/
#define uSPTICK	20000		/* microseconds per clock tick	*/
#define mSPTICK 20		/* milliseconds per clock tick	*/

extern char asy[];		/* mapped area fro sdu io devices */

long	trixtime;
long	timetick;

struct pit	{		/* Intel 8253 */
	char	pit_cnt0;
	char	:8;
	short	:16;
	char	pit_cnt1;
	char	:8;
	short	:16;
	char	pit_cnt2;
	char	:8;
	short	:16;
	char	pit_cmd;
	char	:8;
	short	:16;
};


/* Stuff for timer handler, created by CREAT call to clock */

extern struct list timlist;
extern struct list calllist;


/* system-clock interrupt handler */

clk_rint(dn)
{
	register struct	outcall	*ocp;
	extern	char	DOSCHED;

	if((++timetick & 07) == 0)
		/* force low priority (SPL_WAKEUP) reschedule interrupt */
		DOSCHED = 1;
	if(timetick < HZ)
		return;
	timetick -= HZ;
	trixtime++;
}


/* initialize system-clock */

clk_init()
{
	register struct pit *tp;
	register struct	outcall	 *ocp;


	/* setup counter-0 as system-clock for 20-millisecond interval */
	tp = (struct pit *)&asy[0x160];		/* pit1 */
	tp->pit_cmd = 0x36;			/* C0, mode 3 */
	tp->pit_cnt0 = 0xFF;
	tp->pit_cnt0 = 0x5F;			/* should give use 20ms */

	timetick = 0;
	trixtime = 0;

	/* init timeout list */
	l_list(&timlist);
	l_list(&calllist);			/* init callout list */
}


static	long
clk_gstat(passport, type, dflt)
int	type;
long	dflt;
{
	extern	long	trixtime;

	switch(type) {
	    case STATUS_ATIME:
	    case STATUS_CTIME:
	    case STATUS_MTIME:
		return(trixtime);

	    case STATUS_IDENT:
		return(1);

	    case STATUS_PROT:
		return(PROT_STREAM);

	    case STATUS_ACCESS:
		return(0622);

	    case STATUS_OWNER:
		return(0);

	    case STATUS_SUSES:
		return(0);

	    case STATUS_ISIZE:
		return(sizeof(struct timeb));
	}

	return(dflt);
}


static	long
clk_pstat(passport, type, val)
int	 type;
long	 val;
{
	/* ignore PUTSTATs */
	return(val);
}


/* system-clock handler routine */

clk_handler(passport, mp)
register struct	message *mp;
{
	switch(mp->m_opcode) {
	    case CONNECT:
		mp->m_ercode = E_ASIS;
		return(0);

	    case GETSTAT:
	    case PUTSTAT:
		mp->m_ercode = simplestat(mp, clk_gstat, clk_pstat, passport);
		return(0);

	    case READ:
	 	if(mp->m_dwsize == sizeof(struct timeb)) {
			struct	timeb	tm;

			tm.time = trixtime;
			tm.millitm = timetick * mSPTICK;
			tm.timezone = (5*60);
			tm.dstflag = 1;
			mp->m_param[0] =
			    kstore(mp->m_dwaddr, &tm, mp->m_dwsize);
		}
		else if(mp->m_dwsize == sizeof(long))
			mp->m_param[0] =
			    kstore(mp->m_dwaddr, &trixtime, mp->m_dwsize);
		else
			mp->m_param[0] = 0;
		break;

	    case WRITE:
		if(mp->m_dwsize == sizeof(struct timeb)) {
			struct	timeb	tm;

			mp->m_param[0] =
			    kfetch(mp->m_dwaddr, &tm, mp->m_dwsize);
			trixtime = tm.time;
			timetick = 0;
		}
		else if(mp->m_dwsize == sizeof(long))
			mp->m_param[0] =
			    kfetch(mp->m_dwaddr, &trixtime, mp->m_dwsize);
		else
			mp->m_param[0] = 0;
		break;

	    default:
		mp->m_ercode = E_OPCODE;
		return(0);
	}

	mp->m_ercode = E_OK;

	/* the request is completed */
	return(0);
}


/* return handle to system-clock object */

hndl_t
clk_make()
{
	return(p_kalloc(clk_handler, 0, 0));
}
