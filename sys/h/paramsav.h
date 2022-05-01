/* size of kernel allocations */

#define	DOMAINS		20
#define THREADS		100
#define PORTS		200
#define HANDLES		400
#define	SEGMENTS	60


/* constants dealing with the message call mechanism */
#define	N_ARGS		15		/* registers saved in thread	*/


#define	SMODE		0x2000		/* supervisor mode bit */
#define	USERMODE(ps)	(((ps) & SMODE)==0)

#define	INTPRI	0x700			/* Priority bits */
#define	BASEPRI(ps)	(((ps) & INTPRI) != 0)

#define	SPL_WAKEUP	0x300		/* block wakeup interrupts */
#define	SPL_SERIAL	0X300		/* highest serial i/o priority */
#define	SPL_DISK	0x300		/* disk i/o priority */
#define	SPL_NET		0x300		/* network i/o priority */
#define	SPL_CLOCK	0x500		/* clock priority?? */


/* constants dealing with the user address space */
#define PAGESIZE	1024
#define PGOFSET		(1024 - 1)
#define PAGEMASK	(PAGESIZE - 1)
#define	PGSHIFT		10
#define	PAGESHIFT	10

#define	STACKSIZE	(32*PAGESIZE)	/* size of stack allocation */
#define	STACKMIN	(2*PAGESIZE)	/* minimum stack allocation */
#define STACKBASE	(caddr_t)(0x600000)
#define	STACKTOP	(caddr_t)(STACKBASE + 0x200000) /*  stack */
#define	USERBASE	(caddr_t)(0x40000)	/* address of text entry */
extern	char	krequest;	/* entry to kernel handlers */
#define	KERNBASE	(caddr_t)(&krequest)	/* address of kernel entry */
#define MAXMEM		8*256*1024

#define	DWBASE		(caddr_t)(0x800000)	/* base of data window seg */
#define	DWTOP		(caddr_t)(DWBASE + 0x200000)

#define UPAGES	7
#define SYSTEM	0xf00000
#define UCHUNK	1024*7

/* type definitions for kernel */

/*
 *	there is a fairly strong assumption in this code that a virtual address
 *	and a physical address are the same length.
 *	this will make it difficult to map the code onto a 16 bit machine
 *	without a great deal of effort.
 */

typedef	long	time_t;

typedef	int	arg_t;			/* type of arguments passed in reg */

typedef	long	pte_t;
typedef	long	mem_t;
typedef	long	off_t;
#define	NULLOFF	((off_t)-1)
typedef	int	(*funp_t)();
typedef	char	*caddr_t;
#define	NULL	0


typedef	struct	handle	handle_t;
#define	NULLHANDLE	((handle_t *)0)

typedef struct	port	port_t;
#define	NULLPORT	((port_t *)0)

typedef	struct	domain	domain_t;
#define	NULLDOMAIN	((domain_t *)0)

typedef	struct	subdom	subdom_t;
#define	NULLSUBDOM	((subdom_t *)0)

typedef	struct	thread	thread_t;
#define	NULLTHREAD	((thread_t *)0)

typedef	struct	segment	segment_t;
#define	NULLSEGMENT	((segment_t *)0)

typedef	struct	link	link_t;
#define	NULLLINK	((link_t *)0)


#ifdef DEBUG
/* DEBUGOUT((arg1, arg2, ...))  --> debug(arg1, arg2, ...) */
#define	DEBUGOUT(x)	debug x
#else
/* no debugging */
#define	DEBUGOUT(x)
#endif
