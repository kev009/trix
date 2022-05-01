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


extern	char asy[];
int Vid = 0;
putchar(c)
char c;
{
	char *pcistat,*pcidata;
	if(Vid){
		vid_char(c);
	}
	else{
		pcidata = (char *)(&asy[0x150]);
		pcistat = pcidata + 4;
		while((*pcistat & 0x1) == 0);
		*pcidata = c;
	}
	if(c == '\n') putchar('\r');
}

pciput(c)
char c;
{
	char *pcistat,*pcidata;
		pcidata = (char *)(&asy[0x150]);
		pcistat = pcidata + 4;
		while((*pcistat & 0x1) == 0);
		*pcidata = c;


}

pciget()
{
	char *pcistat,*pcidata;
	pcidata = (char *)(&asy[0x150]);
	pcistat = pcidata + 4;
	while((*pcistat & 0x2) == 0);
	return(*pcidata);


}

/*
 * PUTSTR - force write a character string to system-console,
 *	    used by printf() within the Trix kernel.
 */
putstr(string, count, adjust, fillch)
register char	*string;
register count;
register adjust;
{

	/* output formatted character string		*/
	while (adjust < 0)
	{
		if (*string=='-' && fillch=='0')
		{
			putchar(*string++);
			count--;
		}
		putchar(fillch);
		adjust++;
	}
	while (--count>=0)
		putchar(*string++);
	while (adjust)
	{
		putchar(fillch);
		adjust--;
	}


}
