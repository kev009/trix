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
#include	"../h/status.h"
#include	"../h/protocol.h"
#include	"../h/silo.h"
#include	"pci.h"


/*
 * 	Table of counter values to be used to initialize
 *	the pit on the sdu for the 'ah' driver.  The ispeed
 *	value (0 to F) indexes into this table to use
 *	appropriate high & low bytes.
 */

static	struct {
	int	msb;
	int	lsb;
}  iospeed[] = {
	0,	0,		/* zero baud rate 0 */
	6,	0,		/* 50 baud */
	4,	0,		/* 75 baud */
	2,	0xBA,		/* 110 baud (approximate) */

	2,	0x3D,		/* 134.5 baud (approximate) */
	2,	0,		/* 150 baud */
	1,	0x80,		/* 200 baud */
	1,	0,		/* 300 baud */

	0,	0x80,		/* 600 baud */
	0,	0x40,		/* 1200 baud */
	0,	0x2b,		/* 1800 baud (approximate) */
	0,	0x20,		/* 2400 baud */

	0,	0x10,		/* 4800 baud */
	0,	0x08,		/* 9600 baud */
	0,	0x04,		/* 19200 baud */
	0,	0		/* EXTB extras ? */
};


int	pci_rint();
int	pci_xint();
int	pci_handler();


struct	silo	silo0r, silo1r;
struct	silo	silo0x, silo1x;
long	silo0tim, silo1tim;


struct	serial	{
	struct	pci	*addr;		/* hardware register address */
	int	state;			/* state of line */
	int	speed;			/* speed of line */
	struct	silo	*rsilo, *xsilo;	/* the input and output silos */
	long	*timer;
	int	pad1, pad2;
}   pciline[2] = {
	{ (struct pci *)&asy[0x150], 0, -1, &silo0r, &silo0x, &silo0tim },
	{ (struct pci *)&asy[0x158], 0, -1, &silo1r, &silo1x, &silo1tim }
};

#define	SERIAL_INIT	1
#define	SERIAL_IWAIT	2
#define	SERIAL_OWAIT	4


/* initialize PCI-device and associated serial-character-line */

pci_init(dev)
{
	register struct	serial	*pp = &pciline[dev];
	register struct pci	*pci = pp->addr;

	if(pp->state == 0) {
		pp->state = SERIAL_INIT;

		/* initialize silos */
		pp->rsilo->in = pp->rsilo->out = &pp->rsilo->buf[0];
		pp->xsilo->in = pp->xsilo->out = &pp->xsilo->buf[0];
		pp->rsilo->dev = pp->xsilo->dev = (char *)pci;

		/* put pci in known state */
		pci->stat = 0;
		pci->stat = 0;
		pci->stat = 0;
		pci->stat = PCI_RST;		/* initialize with reset command */

		/* Mode: 1 stop bit, 8 bit data, 16x clock, no parity */
		pci->stat = PCI_1BIT | PCI_8BIT | PCI_16X;
		pci->stat = PCI_RTS|PCI_ERST|PCI_RXEN|PCI_TXEN|PCI_DTR;

		pci_speed(dev, 13);		/* set speed to 1200 */
	}
}


/* set pci's speed */

pci_speed(dev, speed)
{
	register struct	I8253	*pit = (struct I8253 *)&asy[PIT_0];

	if(speed != pciline[dev].speed) {
		pit->pit_cmd = ((dev << 6) & 0xc0) | 0x36;
		pit->p_pit[dev].pit_cnt = iospeed[speed].lsb;
		pit->p_pit[dev].pit_cnt = iospeed[speed].msb;

		pciline[dev].speed = speed;
	}
}


/*  make associated serial-character-line handler for PCI-device */

pci_make(dev)
{
	pci_init(dev);
	return(p_kalloc(pci_handler, dev, 0));
}


static	long
pci_gstat(dev, type, dflt)
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
		return((long)(&pciline[dev]));

	    case STATUS_PROT:
		return(PROT_STREAM);

	    case STATUS_ACCESS:
		return(0622);

	    case STATUS_OWNER:
		return(0);

	    case STATUS_SUSES:
		return(0);

	    case STATUS_OSIZE:
		return(SILO);

	    case STATUS_ISIZE:
		return(0);

	    case STATUS_BANDW:
		if(pciline[dev].speed == 9)
			return(1200);
		else if(pciline[dev].speed == 13)
			return(9600);
	}

	return(dflt);
}


static	long
pci_pstat(dev, type, val)
int	 type;
long	 val;
{
	switch(type) {
	    case STATUS_BANDW:
		if(val == 1200)
			pci_speed(dev, 9);
		else if(val == 9600)
			pci_speed(dev, 13);
		break;
	}
	return(val);
}


pci_handler(dev, mp)
register struct	message	*mp;
{
	switch(mp->m_opcode) {
	    case CONNECT:
		mp->m_ercode = E_ASIS;
		return(0);

	    case CLOSE:
		printf("pci_close(%d)\n", dev);
		break;

	    case GETSTAT:
	    case PUTSTAT:
		mp->m_ercode = simplestat(mp, pci_gstat, pci_pstat, dev);
		return(0);

	    case READ:
	    {
		register struct	silo	*silo = pciline[dev].rsilo;

		/* wait for at least one char */
		if(SILOEMPTY(silo) || *pciline[dev].timer) {
			if(krerun() && kwakeval() == RECALL) {
				mp->m_param[0] = 0;
				break;
			}
			pciline[dev].state |= SERIAL_IWAIT;
			ksleep(0, silo);
			return(1);
		}

		mp->m_param[0] = 0;

		/*
		 * move contiguous blocks until read is satisfied or there is no more
		 * new data can arrive at any time
		 */
		while(!SILOEMPTY(silo) && mp->m_dwsize > 0) {
			register char	*end;

			/* find the end of the maximum contiguous block in the silo */
			if((end = silo->in) < silo->out)
				end = &silo->buf[SILO];
			if(end > &silo->out[mp->m_dwsize])
				end = &silo->out[mp->m_dwsize];

			kstore(mp->m_dwaddr, silo->out, end - silo->out);

			/* update the data window for next time through the loop */
			mp->m_param[0] += end - silo->out;
			mp->m_dwaddr += end - silo->out;
			mp->m_dwsize -= end - silo->out;

			/* free the space in the silo */
			if(end == &silo->buf[SILO])
				end = &silo->buf[0];
			silo->out = end;
		}
		if(mp->m_dwsize > 0)
			/* after a partial read force a timeout on the silo */
			*pciline[dev].timer = 2;
		if(mp->m_param[0] < 3) {
			extern	char	DOSCHED;
			/* force low priority (SPL_WAKEUP) reschedule interrupt */
			DOSCHED = 1;
		}

		break;
	    }

	    case WRITE:
	    {
		register struct	silo	*silo = pciline[dev].xsilo;

		if(!krerun()) {
			/* only reset write count on first time through */
			mp->m_param[0] = 0;
		}
		else if(kwakeval() == RECALL) {
			/* if we were aborted out of a sleep */
			break;
		}

		/* move contiguous blocks until write is satisfied */
		while(mp->m_dwsize > 0) {
			register char	*end;

			/* wait for some space */
			if(SILOFULL(silo)) {
				pciline[dev].state |= SERIAL_OWAIT;
				ksleep(0, silo);
				return(1);
			}

			/* find the end of the maximum contiguous hole in the silo */
			if((end = &silo->out[-1]) < silo->in)
				end = &silo->buf[SILO];
			if(end > &silo->in[mp->m_dwsize])
				end = &silo->in[mp->m_dwsize];

			kfetch(mp->m_dwaddr, silo->in, end - silo->in);

			/* update the data window for next time through the loop */
			mp->m_param[0] += end - silo->in;
			mp->m_dwaddr += end - silo->in;
			mp->m_dwsize -= end - silo->in;

			/* use the space in the silo */
			if(end == &silo->buf[SILO])
				end = &silo->buf[0];

			if(SILOEMPTY(silo)) {
				/* it was empty, restart it */
				silo->in = end;
				pci_xint(dev);
			}
			else
				silo->in = end;
		}
		break;
	    }

	    default:
		mp->m_ercode = E_OPCODE;
		return(0);
	}

	mp->m_ercode = E_OK;
	return(0);
}


/*
 *  receiver interrupt routine for pci
 *  wakeup sleeping threads (those waiting for data) who will then empty silo
 */

pci_rint(dev)
{
	register struct	pci	*pci = pciline[dev].addr;

	if(pciline[dev].state & SERIAL_IWAIT) {
		pciline[dev].state &= ~SERIAL_IWAIT;
		kwakeup(0, pciline[dev].rsilo);
	}

	if(pci->stat & PCI_OE)
		/* can this screw up if done at low priority? */
		pci->stat = PCI_RTS|PCI_ERST|PCI_RXEN|PCI_TXEN|PCI_DTR;
}


/*
 *  pseudo-dma completion interrupt routine for pci
 *  wakeup sleeping threads (those with pending data) who will then fill silo
 *  restart pseudo-dma
 */

pci_xint(dev)
{
	register struct silo	*silo = pciline[dev].xsilo;
	register struct	pci	*pci = pciline[dev].addr;

	if(pci->stat & PCI_TXRDY) {
		if(!SILOEMPTY(silo)) {
			pci->data = *silo->out;
			if(&silo->out[1] == &silo->buf[SILO])
				silo->out = &silo->buf[0];
			else
				silo->out++;
		}
		else if(pciline[dev].state & SERIAL_OWAIT) {
			pciline[dev].state &= ~SERIAL_OWAIT;
			kwakeup(0, silo);
		}
	}
}
