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



/* Function keys at the top of the keyboard */

#define F1	0x03	/* F1 key */
#define F2	0x04	/* F2 key */
#define F3	0x05	/* F3 key */
#define F4	0x06	/* F4 key */

#define F5	0x08	/* F5 key */
#define F6	0x09	/* F6 key */
#define F7	0x0A	/* F7 key */
#define F8	0x0B	/* F8 key */

#define F9	0x0C	/* F9 key */
#define F10	0x0D	/* F10 key */
#define F11	0x0E	/* F11 key */
#define F12	0x0F	/* F12 key */

/* Numeric keypad */

#define NEQUAL	0x32	/* numeric keypad equal key */
#define NPLUS	0x33	/* numeric keypad plus key */
#define NSPACE	0x34	/* numeric keypad space key */
#define NTAB	0x35	/* numeric keypad tab key */
#define NMINUS	0x4B	/* numeric keypad minus key */
#define NQUOTE	0x63	/* numeric keypad quote key */
#define NENTER	0x7F	/* numeric keypad enter key */
#define NPERIOD	0x7E	/* numeric keypad period key */

#define	N0	0x7D	/* numeric 0 key */
#define	N1	0x76	/* numeric 1 key */
#define	N2	0x77	/* numeric 2 key */
#define	N3	0x78	/* numeric 3 key */
#define	N4	0x60	/* numeric 4 key */
#define	N5	0x61	/* numeric 5 key */
#define	N6	0x62	/* numeric 6 key */
#define	N7	0x48	/* numeric 7 key */
#define	N8	0x49	/* numeric 8 key */
#define	N9	0x4A	/* numeric 9 key */

/* Arrow keys */

#define AUP	0x47	/* arrow up key */
#define ADOWN	0x75	/* arrow down key */
#define ALEFT	0x5D	/* arrow left key */
#define ARIGHT	0x5F	/* arrow right key */
#define AHOME	0x5E	/* arrow home key */

#define	INS	0x14	/* INS key */
#define	LF	0x15	/* LF key moved by popular demand */
#define	BRKPS	0x16	/* BRK/PAUS (reboot) key */
#define	PRNT	0x17	/* PRNT (debugging printout) key */

#define CNTL	0x4E	/* character number of control key */
#define SHIFT1	0x67	/* character number of left shift */
#define SHIFT2	0x72	/* character number of right shift */
#define CAPS	0x4F	/* character number of caps lock */

#define CMDWAIT	10	/* ticks to wait for a keyboard to response */

			/* keyboard flags */
#define K_CAPS	0x2	/* => caps lock pressed */
#define K_ESC	0x4	/* => we have seen an escape char */
#define K_OK	0x8	/* => positive acknowledgement received */
#define K_CLICK 0x10	/* => key click on */

#define UP	0x0	/* key is in up position */
#define DOWN	0x1	/* key is in down position */
