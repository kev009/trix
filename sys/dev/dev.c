#include "../h/param.h"
#include "../h/args.h"
#include "../h/calls.h"
#include "../h/nuaddr.h"
#include "../h/config.h"

/*
 *	TRIX 1.0
 *	Initialize Devices and Administer Device Interrupts
 */

struct	icatch
{
	long	ic_smoveml;	/* save registers	*/
	short	ic_movl;	/* movl instruction	*/
	int	ic_arg;		/*     argument value	*/
	short	ic_jsr;		/* jsr instruction	*/
	int	(*ic_handler)();/*     handler routine	*/
	short	ic_addql;	/* addql instruction	*/
	long	ic_rmoveml;	/* restore registers	*/
	short	ic_rte;		/* rte instruction	*/
};

extern	int	_ignore();
extern	struct	icatch	_iproto;
struct	icatch	_icatch[N_DEVI];
extern	int	_spltrap();

/*
 * DEV_IALLOC - allocate an interrupt-catch structure
 */
struct	icatch	*
dev_ialloc()
{
	register struct icatch	*ip;

	for (ip = &_icatch[0]; ip < &_icatch[N_DEVI]; ip++)
		if (ip->ic_handler == 0)
			return(ip);
	panic("cannot allocate _icatch structure");
	return(0);
}

/*
 * DEV_IMATCH - attempt to match an interrupt-catch structure
 */
struct	icatch	*
dev_imatch(mip)
register struct	icatch	*mip;
{
	register struct icatch	*ip;

	for (ip = &_icatch[0]; ip < &_icatch[N_DEVI]; ip++)
		if (mip == ip)
			return(ip);
	return(0);
}

/*
 * DEV_ISET - setup an interrupt vector 'ivec' to point to a catch routine
 *	      that will call 'ihandler' with 'iarg' as argument
 */
dev_iset(ioffset, ihandler, iarg)
int	(*ihandler)();
{
	register int	s, *ivec;
	register struct	icatch	*ip;

	ivec = 0;
	s = spl7();
	if (ihandler != 0)
	{
		if ((ip = dev_imatch(*ivec)) == 0)
		{
			ip = dev_ialloc();
			ivec[ioffset] = (int)ip;
		}
		ip->ic_handler = ihandler;
		ip->ic_arg = iarg;
	}
	else
	{
		if ((ip = dev_imatch(ivec[ioffset])) != 0)
			ip->ic_handler = 0;				
		ivec[ioffset] = (int)_ignore;
	}
	splx(s);
}

/*
 * DEV_INIT - initialize all devices that need it
 */
dev_init()
{
	register i;
	register struct	icatch	*ip;

	/*
	 * initialize _icatch routines
	 */
	for (ip = &_icatch[0]; ip < &_icatch[N_DEVI]; ip++)
	{
		*ip = _iproto;
		ip->ic_handler = 0;
	}

	/*
	 * setup spl() special trap
	 */
	spl_tset();

	/*
	 * initialize devices
	 */
	iob_init();
	pci_init();
	clk_init();
	prf_init();
	dk_init();

	/*
	 * enable interrupts
	 */
	spl0();
}
