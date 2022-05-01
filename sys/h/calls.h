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


#define hndl_t	long
#define NULLHNDL	0


/* trix kernel calls	*/
#define	K_ERROR		0
#define	K_REQUEST	1
#define K_REPLY		2
#define	K_RELAY		3
#define	K_FETCH		4
#define	K_STORE		5
#define	K_SUBDOM	6
#define	K_SPAWN		7
#define K_MAKEPORT	8
#define K_DUP		9
#define	K_FOLD		10
#define	K_CLOSE		11
#define K_RECALL	12
#define K_SPL		13

extern	hndl_t	t_MAKEPORT();
extern	hndl_t	t_DUP();
extern	hndl_t	t_LOOKUP();
extern	hndl_t	t_MOUNT();
extern	hndl_t	t_CREAT();


/* trix reply states */
#define	E_OK		0	/* no errors */
#define	E_NPORT		1	/* out of ports */
#define	E_NHANDLE	2	/* out of handles */
#define	E_HANDLE	3	/* attempt to use bad handle */
#define	E_SIZE		4	/* SIZE of domain error ??? */
#define	E_SPAWN		5	/* can't spawn thread */
#define	E_LOOKUP	6	/* bad open message */
#define	E_OPCODE	7	/* bad opcode of message */
#define	E_WINDOW	8	/* bad data window */
#define E_NDOMAIN	9	/* out of domains */
#define E_NOBJECT	10	/* cannot find object */
#define E_EINVAL	11	/* invalid message value */
#define	E_SPACE		12	/* out of space */
#define	E_MAXSIZE	13	/* can't expand further */
#define	E_TARGET	14	/* bad target to request/jump */
#define	E_EOF		15	/* end of data */
#define E_NOEXEC	16	/* exec file format error */
#define	E_ENTER		17	/* bad enter */
#define	E_FOLD		18	/* cant fold handle to local id */
#define E_NOLOAD	19	/* cannot load domain */
#define	E_CONNECT	20	/* can't connect in the specified way */
#define	E_ASIS		21	/* the connect should use existing handle */
#define	E_TOOBIG	22	/* the data window is too big */
#define	E_STATPARSE	23	/* the stat buffer doesn't parse */


/* masks for opcode extraction	*/
#define	OP_SYSTEM	0x80000000	/* opcodes only the kernel can use */
#define OP_FERROR	0x80000000	/* recall on reply */
#define OP_HANDLE	0x40000000	/* passed handle bit */
#define OP_DWTYPE	0x30000000	/* dwtype bit mask */
#define OP_OPCODE	0x0FFFFFFF	/* message opcode mask */


/* data window types */
#define	DW_READ		0x10000000	/* READable window */
#define DW_WRITE	0x20000000	/* WRITEable window */


#define	PARSESIZE  256		/* maximum length of name element */

/* request opcodes */
#define	CLOSE	(OP_SYSTEM | 0x1)

#define	LOOKUP	(DW_READ | 0x2)
#define	LOOKDIR	(DW_READ | 0xB)
#define	CONNECT	(0x2)
#define		CONNECT_ASIS	0	/* return any available handle */
#define		CONNECT_RDONLY	1	/* access data but not modify it */
#define		CONNECT_WRONLY	2	/* modify data but not access it */
#define		CONNECT_RDWR	3	/* access and modify data */
#define		CONNECT_TRUNC	4	/* truncate data (if appropriate) */
#define		CONNECT_APPEND	8	/* allow data to be appended only */
#define		CONNECT_LOCK	16	/* interlock access to the data */
#define		CONNECT_SNPSHT	32	/* return stable snapshot of the data */
#define		CONNECT_CREAT	64	/* creat the object if it doesn't exist */

/*#define STAT	(DW_WRITE | 0x3)		/* replaced by STATUS */

#define	READ	(DW_WRITE | 0x4)
#define	WRITE	(DW_READ | 0x5)
#define		APPEND_OFF	(off_t)(-1)	/* append to end of file */

#define	SLEEP	(0x6)
#define	WAKEUP	(0x7)

#define	CREAT	(DW_READ | 0x8)

#define ENTER	(OP_HANDLE | DW_READ | 0x9)
#define DELETE	(DW_READ | 0xA)
#define SOFTENTER (OP_HANDLE | DW_READ | 0xB)

#define	MOUNT	(OP_HANDLE | 0xD)

/*#define CNTRL	(DW_READ | DW_WRITE | 0xE)*/

#define UPDATE	(0x11)

#define	GO	(OP_HANDLE | DW_READ | 0x16)
#define	LOAD	(OP_HANDLE | 0x17)

#define	RECALL	(0x1A)

#define	GETSTAT	(DW_READ | DW_WRITE | 0x1C)
#define	PUTSTAT	(DW_READ | DW_WRITE | 0x1D)


/* trix message structure used by K_REQUEST, K_JUMP, K_REPLY */
struct	message	{
	arg_t	m_opcode;		/* message opcode */
#define	m_ercode m_opcode		/* reply errcode */
	hndl_t	m_handle;		/* passed handle-id */
	arg_t	m_param[4];		/* passed parameters */
			/* data window (not used by K_REPLY) */
	caddr_t	m_dwaddr;		/* data window address */
#define	m_dwoffs m_dwaddr		/* data window offset */
#define	m_dwbase m_dwaddr		/* data window base */
	off_t	m_dwsize;		/* data window size */
	funp_t	m_entry;		/* entry function */
	long	m_uid;			/* user id of requester */
#define	m_path	m_uid			/* protection value of return path */
};


/* definitions of excpetion codes */

#define	EXCEPT_RECALL	1		/* recall exception */
#define	EXCEPT_DETATCH	2		/* detatch exception */
#define	EXCEPT_BUSERR	3		/* bus/address error exception */
#define	EXCEPT_MATH	4		/* math exception */
