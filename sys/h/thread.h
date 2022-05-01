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
 *  a thread is the basis of activity in TRIX
 *  it is represented by its request stack of (struct thread) elements
 *
 *  an activity is a group of threads linked together through their request
 *    stacks
 *
 *  in spawning a new thread we initially simulate a call on a port to the
 *    current domain whose handler is (*fun)() so that when the thread starts
 *    running it will be started in the desired place
 *  when the call returns the thread is free
 *
 *  note that there is no notion of a thread name
 *    all references to threads are via the activity hierarchy (ie you can
 *    refer to the current thread or its direct children)
 *  this seems to suffice for signaling but doesn't solve killing a runaway
 *    thread which will have to use a more adhoc scheme
 *  at least the "kill" mechanism can also kill the activity completely
 */


struct	thread	{
	/* this stuff must not move to interface with low.s */
	struct	subdom	*t_subdom;	/* domain thread is in */

	union	{
		arg_t	T_ARGS[N_ARGS];		/* (saved regs) call arguments */
		struct	{
			arg_t	T_KERCALL;
			arg_t	T_TARGET;
			struct	message	T_MSG;		
			arg_t	T_ENTRY;
		} t_mesg;			/* template used for request/reply */
	} t_data;
#define	t_msg		t_data.t_mesg.T_MSG
#define	t_kercall	t_data.t_mesg.T_KERCALL
#define	t_target	t_data.t_mesg.T_TARGET
#define	t_passport	t_data.t_mesg.T_TARGET
#define	t_entry		t_data.t_mesg.T_MSG.m_entry
#define	t_args		t_data.T_ARGS

	caddr_t	t_usp;			/* user stack-pointer */

	struct exception {		/* excepetion stack for 68010 */
		short	ex_ups;		/* user psw */
		int	(*ex_upc)();	/* user pc */
		short	ex_type:4;	/* exception type (high bits) */
		short	ex_vect:12;	/* exception vector (low bits) */

		/* the rest is for type 8 exceptions */
		short	ex_ssw;		/* special status word */
		caddr_t	ex_faddr;	/* faulting address */
		short	:1;		/* UNUSED, RESERVED */
		short	ex_dout;	/* data output buffer */
		short	:1;		/* UNUSED, RESERVED */
		short	ex_din;		/* data input buffer */
		short	:1;		/* UNUSED, RESERVED */
		short	ex_inst;	/* instruction input buffer */
		short	ex_other[16];	/* the rest of the micro state */
	} t_except;

	caddr_t	t_stacktop;		/* top of user stack */
	caddr_t	t_watch;		/* watched location with TBIT on */

	/* state of the thread */
	short	t_state;		/* execution state of thread */
	long	t_opcode;		/* opcode of current request */
	union	{
	    struct	{
		caddr_t t_saddr;	/* sync sleep address */
		int	t_syncn;	/* syncronizer identity */
		struct	link	t_slnk;	/* link in sleeping threads list */
	    }	t_sleep;		/* T_SLEEPING info */
	    int	    t_wakeup;		/* cause of wakeup */
	    struct	{
		long	t_timout;	/* timout delay	*/
	    }	t_timer;		/* T_TIMING info */
	    struct	{
		caddr_t	t_paddr;	/* address of needed page */
	    }	t_page;		/* T_PAGING info */
	} t_wait;			/* why not T_RUNNING */

	/* information for current request */
	short	t_priority;		/* software priority level */
	struct	link	t_schdlnk;	/* next thread in current domain */

	/* information describing the data window */
	struct	map	t_dwmap;	/* map of windowed segment */
	caddr_t	t_dwbase;		/* start of window on segment */
	caddr_t	t_dwbound;		/* end of window on segment */

	struct	thread	*t_stack;	/* previous request stack */
	struct	thread	*t_rstack;	/* next request stack element */

	time_t	t_time;			/* accumulated time */

	/* information describing user stack */
	struct	map	t_smap;

	/* information describing activity group of thread */
	struct	thread	*t_parent;	/* spawning thread element */
	struct	list	t_children;	/* first of children */
	struct	link	t_sibling;	/* next child of parent */

	struct	domain	*t_close;	/* domain receiving close rqst */

	long	t_uid;			/* user id of running thread */

	/* information for dealing with exceptions */
	void	(*t_exhand)();		/* exception handler */
	long	t_exignor;		/* mask of exceptions that are ignored */
	long	t_expend;		/* mask of pending exceptions */
};

#define	t_upc	t_except.ex_upc
#define	t_ups	t_except.ex_ups
#define T_args	T_CURRENT->t_args


/* states for a thread */
#define	T_FREE		0
#define	T_INUSE		(1<<15)		/* a bit you can always leave set */
#define	T_RUNNING	(1<<0)		/* is currently running */
#define	T_RUNABLE	(1<<1)		/* is runnable but stopped */
#define	T_SWAPPED	(1<<2)		/* is swapped */
#define	T_PAGING	(1<<3)		/* is waiting on page fault */
#define	T_SLEEPING	(1<<4)		/* is sleeping on a value */
#define	T_TIMING	(1<<5)		/* is waiting for a timeout */
#define	T_REQUEST	(1<<6)		/* is doing a request */
#define	T_CLOSE		(1<<7)		/* is some domains close thread */
#define	T_HASRUN	(1<<8)		/* has run (for priority) */
#define	T_RECALLED	(1<<9)		/* has a recall pending */
#define	T_KSLEEPING	(1<<10)		/* is sleeping in a kdev */
#define	T_KREQUEST	(1<<11)		/* has a KREQUEST in progress */
#define	T_KERNEL	(1<<12)		/* is in the midst of a kernel call */


/*
 * A Note on Stacks 3/84:
 *
 *	TRIX uses two stacks at any time :
 *		the u[ser]stack and the k[ernel]stack.
 *
 *	the current implementation of the ustack is in two parallel parts:
 *		a segment that is user accessible and is paged/swapped.
 *		a list of thread elements which contain the kernel protected
 *		  information from request() calls and (since it is not
 *		  paged) is always available without the necessity of IO.
 *
 *	the reasons for not pushing the request stack on the ustack are:
 *	  a paged ustack might result in the call paging
 *	  the request stack is searched in various low level operations
 *	    (eg. garbage collection) its being paged would seriously hamper
 *	    making these operations atomic
 *
 *	the kstack is the interrupt/trap stack.  it is always mapped in
 *	  and (unlike unix) there is only a single kstack per processor.
 *	with one per processor code run on the kstack must be completly
 *	  reentrant with explicit synchronization between the processors.
 *	  if you want to block -- call into system.
 *
 *	on a trap the user state (registers, ...) is saved on in the thread
 *	  data structure.
 *	returning from a trap restores the state (of possibly a different
 *	  thread)
 *
 *	if the trap is a request() the thread stack is pushed and the return
 *	  is to the new domain.
 *	this requires that the stack be remapped (to protect the portion used
 *	  by the caller) and that the called domain be mapped in.
 *
 *	if the trap is a reply() the important portion of the saved state
 *	  is restored from the thread stack (the top of which is freed).
 *	this again requires the stack be remapped (to allow access to the
 *	  portion used by the caller and free extra space used in the call)
 *	  and that the returned to domain be mapped in.
 *
 *	note that the "message" on both request() and reply() are sent in the
 *	  "non-important" user registers.
 */
