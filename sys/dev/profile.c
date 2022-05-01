#include "../h/param.h"
#include "../h/args.h"
#include "../h/calls.h"
#include "../h/nuaddr.h"

/*
 *	TRIX 1.0
 *	Kernel Profiler Timer Routines
 */

struct	iobtimer
{
	char tim_1;		/* filler	*/
	char tim_cnt0;		/* counter 0	*/
	char tim_2[3];		/* filler	*/
	char tim_cnt1;		/* counter 1	*/
	char tim_3[3];		/* filler	*/
	char tim_cnt2;		/* counter 2	*/
	char tim_4[3];		/* filler	*/
	char tim_cntl;		/* control byte */
};

struct	kprof
{
	long	kp_scale;
	long	kp_base;
	long	kp_size;
	char	*kp_ptr;
};

extern	int	_etext;
extern	int	_ikprof();

#define KPBF	11
#define TXTF	15
#define	KPBSIZE	(1<<KPBF)	/*  2K words	*/
#define TXTSIZE	(1<<TXTF)	/* 32K words	*/
unsigned short	kpbuf[KPBSIZE];
struct	kprof	tkprof;
long	totcnt;
long	usrcnt;
long	syscnt;
long	kercnt;
long	scnt;
long	kcnt;
/*
 * PRF_INIT - initialize profile timer
 */
prf_init()
{
	register int	*ivec;
	register struct	kprof	 *kp;
	register struct iobtimer *tp;

	/*
	 * setup profiler stuff
	 */
	kp = &tkprof;
	kp->kp_scale = TXTF-KPBF;	/* 4	*/
	kp->kp_base = (int)&__text;
	kp->kp_size = KPBSIZE<<1;	/* 4K	*/
	kp->kp_ptr = (char *)kpbuf;

	/*
	 * setup counter-1 as profile-clock for 5-millisecond interval
	 */
	tp = (struct iobtimer *)(IOB + TIMER);
	tp->tim_cntl = 0x76;
	tp->tim_cnt1 = 5000 & 0xFF;
	tp->tim_cnt1 = (5000 >> 8) & 0xFF;

	/*
	 * setup interrupt vector for timer counter-1
	 */
	ivec = 0;
	ivec[IOB_EVENT + IOE_TIMR1] = (int)_ikprof;

	/*
	 * setup IOB event-ram for counter-1
	 */
	iob_event(IOE_TIMR1, (IOB_EVENT + IOE_TIMR1), 1);
}

/*
 * PRF_ON - clear profile-buffer and turn on profiler
 */
prf_on()
{
	register unsigned short	*sp;

	printf("kernel profiling on: (textsize = %D bytes)\n",
		(int)&_etext - (int)&__text);
	for (sp = kpbuf; sp < &kpbuf[KPBSIZE]; sp++)
		*sp = 0;
	totcnt = 0;
	usrcnt = 0;
	syscnt = 0;
	kercnt = 0;
	scnt = 0;
	kcnt = 0;
	iob_control(IOC_TIMR1, 1);
}

/*
 * PRF_OFF - turn off profiler and print profile-buffer
 */
prf_off()
{
	register s, addr, incr;
	register unsigned short	*sp;

	s = spl7();
	iob_control(IOC_TIMR1, 0);
	splx(s);

	printf("kernel profiling off: totcnt = %D\n", totcnt);
	printf("\t(usrtime = %D%%) (systime = %D%%) (kertime = %D%%)\n",
		(usrcnt*100)/totcnt, (syscnt*100)/totcnt, (kercnt*100)/totcnt);
	printf("\t(idle = %D%%) (kcalls = %D%%)\n",
				(scnt*100)/totcnt,(kcnt*100)/totcnt);

	addr = tkprof.kp_base;
	incr = 2<<(TXTF-KPBF);
	for (sp = kpbuf; sp < &kpbuf[KPBSIZE]; addr += incr, sp++)
		if (*sp != 0)
			printf("0x%X: %D\n", addr, (int)(*sp & 0xFFFF));
}

