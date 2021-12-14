/* printf				SAW 6/79
 *	as modified from printf by 	rae 1/81
 * Calls only putc(c)
 * Escape sequences implemented:
 *	%nd	1-prec decimal, signed
 *	%nD	2-prec decimal, signed
 *	%no	1-prec octal, unsigned
 *	%nO	2-prec octal, unsigned
 *	%nx	1-prec hex, unsigned
 *	%nX	2-prec hex, unsigned
 *	%s	string
 *	%c	character
 *	%%	percent
 *
 * in above, "n" can be an integer arg or "v", whence field width comes
 *   from arg list.
 */

#define	ESC		'%'
#define	IOB		0x10000L	/* addr. of i/o board */
#define	IOEVRAM		IOB		/* addr. of i/o event ram */
#define	EVNUM		16		/* number of i/o events */
#define	IOCTRL		(IOB+0x60)	/* addr. of i/o ctrl. reg. */
#define UNIX_PCI	(IOB+0xF0)	/* Unix PCI port */
#define	UPCI_DATA	((short *)UNIX_PCI)
#define	UPCI_SR		((short *)(UNIX_PCI+0x4))
#define	UPCI_MR		((short *)(UNIX_PCI+0x8))
#define	UPCI_CR		((short *)(UNIX_PCI+0xC))
#define	PC_RXRDY	0x2		/* receiver ready bit */
#define	PC_TXRDY	0x1		/* transmit ready bit */


extern putc();

long	DecTab[] = { 1000000000L, 100000000L, 10000000L, 1000000L,
		   100000L, 10000L, 1000L, 100L, 10L, 1L, 0L };

RdArg(ptr,where)
 register char *ptr;
 register int *where;
 {	int val = 0;
	while (*ptr >= '0' && *ptr <= '9')
		 val = (val<<3) + (val<<1) + *ptr++ - '0';
	*where = val;
	return (int) ptr; }

int putn2(val,digs)
 register long val;
 register char *digs;
 {	long *lp = DecTab;
	int ndigs = 10, i=0;
	int dig;

	if (val == 0) { digs[0] = 0; return 1; }
	while (*lp > val) { lp++; ndigs--; };
	while (ndigs)
	 { for (dig = 0; (val -= *lp) >= 0; dig++); val += *lp++;
	   digs[--ndigs] = dig; i++; }
	return i; }


putnum(val,fmt,radix,sign)
 register int val;
 {	char digbuf[100], negflag = 0;
	int ndig = 0, spaces = fmt;
	if (sign && val<0) {	if ((val = -val) < 0) val = 0; negflag++; }
	switch (radix) {
		case 10:	ndig = putn2(val,digbuf); break;
		case 16:	do { digbuf[ndig++] = val & 0XFL;
				     val = (val >> 4) & 0X0FFFFFFF; }
				while (val); break;
		case 8:		do { digbuf[ndig++] = val & 0X7L;
				     val = (val >> 3) & 0X1FFFFFFF; }
				while (val); break;
		case 2:		do { digbuf[ndig++] = val & 0X1L;
				     val = (val >> 1) & 0X7FFFFFFF; }
				while (val); break; }

	spaces = fmt-ndig-negflag;
	if (sign) while (spaces-- > 0) putc(' ');
	else while (spaces-- > 0) putc('0');
	if (negflag) putc('-');
	while (ndig)
	 { if ((spaces = digbuf[--ndig]) >= 10) spaces += 'A'-'0'-10;
	   putc(spaces + '0'); }}

putstring(str,fmt)
 register char *str;
 register int fmt;
 {	while (*str && fmt--) putc(*str++);
	while (fmt-- > 0) putc(' '); }

printf(ctl,args)
 char *ctl;
 {	register int *argp = &args;
	register char *cc = ctl;
	register int ch;
	int Arg1, Arg2;

	while (ch = *cc++)
	 { if (ch != ESC) { putc(ch); continue; }
	   Arg1 = -1; Arg2 = -1;
	   if (((ch = *cc++) >= '0' && ch <= '9') || (ch == 'v'))
		{ if (ch == 'v') Arg1 = *argp++;
		  else cc = (char *) RdArg(--cc,&Arg1);
		  if ((ch = *cc++) == '.')
			{ if (*cc == 'v') { Arg2 = *argp++; cc++; }
			  else cc = (char *) RdArg(cc,&Arg2); ch = *cc++; }}
	   switch (ch) {
		case ESC:	putc(ESC); break;
		case 0:		putc(ESC); return;
		case 'D':
		case 'd':	putnum(*argp++,Arg1,10,1); break;
		case 'O':
		case 'o':	putnum(*argp++,Arg1,8,0); break;
		case 'X':
		case 'x':	putnum(*argp++,Arg1,16,0); break;
		case 'c':	putc(*argp++); break;
		case 's':	putstring(*argp++,Arg1); break;

		default:	putc(ESC); putc(ch); break; }}
 }

getc()				/* get a character (from UNIX) */
{	register short c;
	while (!(*UPCI_SR & PC_RXRDY)) ;
	c = (*UPCI_DATA & 0xFF);
	return (c);
}

putc(c)				/* put a character (to UNIX) */
register short c;
 {
	while (!(*UPCI_SR & PC_TXRDY)) ;
	*UPCI_DATA = c;
	return (c);
 }

IO_Init()
{	short ignore;
	register int x;
	register short i;
	register short *p = (short *)IOEVRAM;

	*((short *)IOCTRL) = 0;	/* Turn off I/O interrupts		*/
				/* initialize unix pci			*/
	ignore = *UPCI_CR;	/* Read command reg to cause reset	*/
	*UPCI_MR = 0X4E;	/* 1 stop, 8 data bits, no par, x16 clk */
	*UPCI_MR = 0X3E;	/* 9600 baud				*/
	*UPCI_CR = 0X37;	/* reset errors, enable receiver	*/
	ignore = *UPCI_DATA;	/* just to reset things.		*/

	for (i=0; i<EVNUM; p++, i++)
		*p++ = 0;


}
