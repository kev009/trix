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
 * Header prepended to each a.out file.
 */
struct exec {
	long	a_magic;	/* magic number */
	long	a_text;		/* size of text segment */
	long	a_data;		/* size of initialized data */
	long	a_bss;		/* size of uninitialized data */
	long	a_syms;		/* size of symbol table */
	long	a_trsize;	/* size of text relocation */
	long	a_drsize;	/* size of data relocation */
	long	a_entry;	/* entry point */
};

#define	OMAGIC	0407		/* old impure format */
#define	NMAGIC	0410		/* read-only text */
#define	ZMAGIC	0413		/* demand load format */

/*
 * Macros which take exec structures as arguments and tell whether
 * the file has a reasonable magic number or offsets to text|symbols|strings.
 */
#define	N_BADMAG(x) \
    (((x).a_magic)!=OMAGIC && ((x).a_magic)!=NMAGIC && ((x).a_magic)!=ZMAGIC)

#define	N_TXTOFF(x) \
	((x).a_magic==ZMAGIC ? 1024 : sizeof (struct exec))
#define N_SYMOFF(x) \
	(N_TXTOFF(x) + (x).a_text+(x).a_data + (x).a_trsize+(x).a_drsize)
#define	N_STROFF(x) \
	(N_SYMOFF(x) + (x).a_syms)

/*
 * Format of a relocation datum.
 */
struct relocation_info {
	long	r_address;	/* address which is relocated */
unsigned short	r_symbolnum,	/* local symbol ordinal */
		r_pcrel:1, 	/* was relocated pc relative already */
		r_length:2,	/* 0=byte, 1=word, 2=long */
		r_extern:1,	/* does not include value of sym referenced */
		:4;		/* nothing, yet */
};

/*
 * Format of a symbol table entry; this file is included by <a.out.h>
 * and should be used if you aren't interested the a.out header
 * or relocation information.
 */
struct	nlist {
	union {
		char	*n_name;	/* for use when in-core */
		long	n_strx;		/* index into file string table */
	} n_un;
	unsigned char n_type;	/* type flag, i.e. N_TEXT etc; see below */
	char	n_other;	/* unused */
	short	n_desc;		/* see <stab.h> */
# ifndef pdp11
	unsigned long n_value;	/* value of this symbol (or sdb offset) */
# else
	long n_value;
# endif
};
#define	n_hash	n_desc		/* used internally by ld */

/*
 * Simple values for n_type.
 */
#define	N_UNDF	0x0		/* undefined */
#define	N_ABS	0x2		/* absolute */
#define	N_TEXT	0x4		/* text */
#define	N_DATA	0x6		/* data */
#define	N_BSS	0x8		/* bss */
#define	N_COMM	0x12		/* common (internal to ld) */
#define	N_FN	0x1e		/* file name symbol */

#define	N_EXT	01		/* external bit, or'ed in */
#define	N_TYPE	0x1e		/* mask for all the type bits */

/*
 * Sdb entries have some of the N_STAB bits set.
 * These are given in <stab.h>
 */
#define	N_STAB	0xe0		/* if any of these bits set, a SDB entry */

/*
 * Format for namelist values.
 */
#define	N_FORMAT	"%08lx"
