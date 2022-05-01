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
 *	TRIX kernel output routines
 */

/*
 * Version of C Library printf.
 * Used to print diagnostic information
 * directly on console tty.
 * Since it is not interrupt driven,
 * all system activities are pretty much
 * suspended.
 * Printf should not be used for chit-chat.
 */
printf(fmt, args)
register char *fmt;
{
	register int  *argp = &args;
	register char c, *p, *q;
	register int d, width, ndigit, radix;
	register unsigned n;
	int sign, decpt;
	char t[128], digits[10], zfill, rjust, ndfnd;
	struct {
		short unsigned length;
		char dtype;
		char class;
		char *ptr;
	} desc;

	p = &t[0];
	while (c = *fmt++)
		if (c != '%')
			*p++ = c;
		else {
			if (p != &t[0]) {
				putstr(t, p-t, 0);
				p = &t[0];
			}
			rjust = 0;
			ndigit = 0;
			zfill = ' ';
			if (*fmt == '-') {
				rjust++;
				fmt++;
			}
			if (*fmt == '0') {
				zfill = '0';
				fmt++;
			}
			if(*fmt != '*') { 
				width = 0;
				while ((d = *fmt++ - '0') >= 0 && d <= 9)
					width = width*10+d;
			}
			else {
				fmt++;
				d = *fmt++ - '0';
				width = *argp++;
			}
			ndfnd = 0;
			if ((d += '0') == '.') {
			    if(*fmt != '*') {
				ndigit = 0;
				while ((d = *fmt++ - '0') >= 0 && d <= 9) {
					ndfnd++;
					ndigit = ndigit*10+d;
				}
				d += '0';
			    }
			    else {
				fmt++;
				d = *fmt++;
				ndfnd++;
				ndigit = *argp++;
			    }
			}
			switch (d) {
			case 'l':
			case 'L':
				switch(*fmt++) {
				case 'o': goto loct;
				case 'x': goto lhex;
				case 'd': goto ldec;
				case 'u': goto luns;
				default:
					fmt--;
					goto uns;
				}
			case 'o':
			case 'O':
			loct:	radix = 8;
				n = *argp++;
				goto compute;
			case 'x':
			case 'X':
			lhex:	radix = 16;
				n = *argp++;
				goto compute;
			case 'd':
			case 'D':
			ldec:	radix = 10;
			signed:	if (*argp < 0) {
					*p++ = '-';
					n = -*argp++;
				} else
					n = *argp++;
				goto compute;

			case 'u':
			case 'U':
			luns:
			uns:	n = *argp++;
				radix = 10;
			compute:
				if (n == 0 && ndigit == 0)
					*p++ = '0';
				for (q = &digits[0]; n != 0; n = n/radix)
				{
					d = n%radix;
					*q++ = d + (d<10?'0':'A'-10);
				}
				while (q > &digits[0])
					*p++ = *--q;
				goto prbuf;

			case 'c':
				for (q = (char *)argp++, d = 0; d < 4; d++)
					if ((*p++ = *q++) == 0)
						p--;
			prbuf:	q = &t[0];
			prstr:	if ((d = width - (p - q)) < 0)
					d = 0;
				if (rjust == 0)
					d = -d;
				putstr(q, p-q, d, zfill);
				p = &t[0];
				break;

			case 's':
				if ((q = (char *)*argp++) == 0)
					q = "(null)";
				if ((d = ndigit) == 0)
					d = 32767;
				for (p=q; *p!=0 && --d>=0; p++);
				goto prstr;

			case 'r':
				argp = (int *)*argp;
				fmt = (char *)*argp++;
				break;
/*			case 'f':
				if (ndfnd == 0)
					ndigit = 6;
				q = fcvt(*((double *)argp), ndigit,
					&decpt, &sign); argp += 2;
				if (sign)
					*p++ = '-';
				if ((d = decpt) <= 0)
					*p++ = '0';
				else do {
						*p++ = *q++;
					} while (--d > 0);
				if (d = ndigit)
					*p++ = '.';
				if ((decpt = - decpt) > 0)
					while (--d >= 0) {
						*p++ = '0';
						if (--decpt <= 0)
							break;
					}
				if (d > 0)
					while (--d >= 0)
						*p++ = *q++;
				goto prbuf;

			case 'e':
				if (ndfnd == 0)
					ndigit = 6;
				else
					ndigit += 1;
				q = ecvt(*((double *)argp), ndigit,
					&decpt, &sign); argp += 2;
				if (sign)
					*p++ = '-';
				if (*q == '0')
					decpt += 1;
				*p++ = *q++;
				*p++ = '.';
				for (d = ndigit; --d > 0; *p++ = *q++);
				*p++ = 'e';
				decpt -= 1;
				if (decpt >= 0)
					*p++ = '+';
				else {
					*p++ = '-';
					decpt = -decpt;
				}
				*p++ = (unsigned)decpt/10+'0';
				*p++ = (unsigned)decpt%10+'0';
				goto prbuf;

			case 'g':
				if (ndfnd == 0)
					ndigit = 6;
				gcvt(*((double *)argp), ndigit, p); argp +=2;
				while (*p++ != 0);
				p -= 1;
				goto prbuf;
 */			case '%':
				*p++= '%';
				break;
			}
	}
	if (p != &t[0])
		putstr(t, p-t, 0);

	return(1);
}

extern int Debug;

/*
 * Print out a debugging message
 */
debug(s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
char *s;
{
	if(!Debug)return(0);

	return(printf(s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10));
}


/*
 * Print out a message and loop
 */
panic(s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10)
char *s;
{
	printf("Panic:");
	printf(s, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
	prtrace(&(((int *)&s)[-2]));
	for(;;);
}
