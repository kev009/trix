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


/*
 * serial-character-line parameter structure
 */
struct	sclparm
{
	short	sm_imode;		/* input mode			*/
	short	sm_omode;		/* output mode			*/
	char	sm_ispeed;		/* input speed			*/
	char	sm_ospeed;		/* output speed			*/
};

/*
 * serial-character-line structure
 */
struct	scline
{
	short	sl_state;		/* serial line status flags	*/
	char	*sl_addr;		/* pointer to serial device	*/
	struct	cbuf	sl_cbr;		/* receiver circular buffer	*/
	struct	cbuf	sl_cbx;		/* transmitter circular buffer	*/
	int	(*sl_reset)();		/* device reset routine		*/
	int	(*sl_start)();		/* device start routine		*/
	struct	sclparm	sl_parm;	/* parameter structure		*/
};

#define	sl_imode	sl_parm.sm_imode
#define sl_omode	sl_parm.sm_omode
#define sl_ispeed	sl_parm.sm_ispeed
#define sl_ospeed	sl_parm.sm_ospeed

/*
 * sl_state bits
 */
#define SCLINIT		0x1	/* initialized		*/
#define	SCLSKBD		0x2	/* keyboard		*/
#define SCLSTOP		0x4	/* stopped		*/
#define	SCLEXCL		0x8	/* exclusive-use	*/
#define SCLRLOC		0x10	/* receiver locked	*/
#define SCLXLOC		0x20	/* transmitter locked	*/

/*
 * sl_mode bits
 */
#define RAWLMODE	0x1
#define RKEYMODE	0x2

/*
 * start and stop characters
 */
#define	CTRL(c)	('c' & 037)
#define CSTART	CTRL(q)
#define CSTOP	CTRL(s)

/*
 * line speeds
 */
#define B0	0
#define B50	1
#define B75	2
#define B110	3
#define B134	4
#define B150	5
#define B200	6
#define B300	7
#define B600	8
#define B1200	9
#define	B1800	10
#define B2400	11
#define B4800	12
#define B9600	13
#define EXTA	14
#define EXTB	15
