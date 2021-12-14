#

/* EMACS_MODES: c !fill */

/* ddt68.c
 *
 * Minidebugger for MC68000.
 *
 *			DDT COMMAND LIST
 *
 *         n/     Open word n.  Type contents in current mode
 *          /     Open word pointed to by last typed value
 *         n<CR>  Modify currently opened word to be n.  Close word.
 *         n<LF>  Modify, close, open next word
 *         n^     Modify, close, open previous word
 *         n<TAB> Modify, close, open word pointed to by last typed
 *                address
 *          $S    Change current mode to instruction typeout 
 *          $A    Addresses will be typed as absolute numbers
 *          $C    Change current mode to numeric type out
 *         $nR    Change current radix for $C to n (n read in decimal)
 *          $T    Change current mode to ASCII text output
 *          $H    Change to halfword (byte) mode
 *                Note that the initial modes are $S and $16R.  These
 *                can be temporarily changed by the above commands and
 *                will return to the permanent setting by a carriage
 *                return.  The settings can be permanently changed by
 *                using two altmodes before the mode setting command
 *                ($$T sets permanent ASCII mode)
 *          $W    Change to word mode
 *          $L    Change to longword mode
 *         n[     Open location n, type as a numeric
 *         n]     Open location n, type as an instruction
 *         n!     Open location n, do not type value
 *          ;     Retype currently open word
 *	    =	  Retype currently open word as a numeric
 *	    _	  Retype currently open word as an instruction
 *          n$G   Start executing program at location n.  Default is to
 *                start execution at address in JOBSA (start of program)
 *          v$nB  Set breakpoint n at address v (n from 1 to 8 or can
 *                be omitted)
 *          0$nB  Remove breakpoint n
 *           $B   Remove all breakpoints
 *           $P   Proceed from breakpoint
 *           $1UT Turn on single stepping (break at every instruction)
 *           $UT  Turn off single stepping
 *
 * This differs from pdp-11 DDT in that there is no support for symbolic
 * manipulation, longword accesses are supported, some commands are missing,
 * and instruction disassembly is not provided (yet...)
 *
 */

#include	"ddt68.h"



/* Global Variable Declarations */

extern	int	START ();		/* start location (sort of...) */
extern	int	dotinc;			/* size of current instruction */
static	int	getchar ();		/* our own getchar routine */
long	dot = 0;			/* current address for display etc. */
short	permsize = 2;			/* permanent wordsize for display */
short	permmode = INSTR;		/* permanent display mode */
short	permradix = 16;			/* permanent display radix */
short	size = 2;			/* current wordsize for display */
short	mode = INSTR;			/* current display mode */
short	radix = 16;			/* current display radix */
short	cont = FALSE;			/* true if continuing from bkpt */
short	sstep = FALSE;			/* true if single-stepping */
short	perm = FALSE;			/* change mode permanently flag */
long	tempstack[2];			/* temp. stack for rte to user */

/* The Breakpoint Structure */

struct	bkpt {
	long	bk_addr;		/* bkpt. address */
	short	bk_instr;		/* displaced instruction */
	short	bk_active;		/* breakpoint active flag */
} bkpt[NOBKPT] = {
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0,
	0,0,0
};

struct	bkpt	*curbkpt = NULL;	/* current breakpoint */

/* The Saved User Registers */

long	regsave[18] = {
	0,0,0,0,0,0,0,0,		/* data registers */
	0,0,0,0,0,0,0,			/* address registers 0-6 */
	(long)&tempstack[0],		/* initial stack */
	SUPER,				/* initial sr */
	(long)START			/* initial pc */
};


ddt68 ()

/* Just call the command interpreter, which should never return.
 */

{
	printf ("DDT68 Version 1.0\n\n");
	interp ();
}


bkptrap ()

/* Called on occurrence of a breakpoint trap.  Masks breakpoints,
 * figures out which breakpoint occurred, and displays breakpoint
 * message.  Then jumps off to command interpreter.
 */
{
	register int	i;			/* temp. index */
	
	regsave[PC] -= 2;		/* back up to trapped instr. */
	maskbkpts ();
	for (i = 0; i < NOBKPT; i++)
		if (bkpt[i].bk_addr == regsave[PC]) {
			curbkpt = &bkpt[i];
			printf ("BKPT %d; addr = ", i);
			displong (regsave[PC], radix);
			break;
		}
	if (i >= NOBKPT) {
		printf ("Unknown BKPT; addr = ");
		displong (regsave[PC], radix);
		curbkpt =  NULL;
	}
	printf ("\n");
	interp ();
}


trctrap ()

/* The trace trap handler.  Trace traps occur during single-stepping,
 * and in the process of proceeding from a breakpoint.  If we are
 * single-stepping, just call the command interpreter.  If we are
 * proceeding from a breakpoint, we must reinsert the breakpoint
 * trap instruction which was temporarily removed in proceed, and
 * turn off single stepping before returning to the user's program.
 */
{
	if (sstep) {
		printf ("SSTEP; next addr. = ");
		displong (regsave[PC], radix);
		printf ("\n");
		curbkpt = NULL;
		interp ();
	} else if (cont) {
		cont = FALSE;
		*(short *)(curbkpt->bk_addr) = BKPTRAP;
		curbkpt = NULL;
		regsave[SR] &= ~TRACE;
		rtn ();
	} else {
		maskbkpts ();
		printf ("TRC TRAP; next addr = ");
		displong (regsave[PC], radix);
		printf ("\n");
		curbkpt = NULL;
		interp ();
	}
}


nxmtrap (addr)

/* Handles nonexistent memory/odd address traps.  Displays the address
 * being referenced and calls the interpreter.
 * Arguments: */

long	addr;				/* address of fault */
{
	maskbkpts ();
	printf ("NXM/ODD; addr =  ");
	displong (addr, radix);
	printf ("\n");
	curbkpt = NULL;
	interp ();
}


illtrap ()

/* Handles illegal and unimplemented instruction traps.  Displays the
 * address of the illegal instruction.
 */
{
	maskbkpts ();
	printf ("ILL INS; addr =  ");
	displong (regsave[PC], radix);
	printf ("\n");
	curbkpt = NULL;
	interp ();
}


badtrap ()

/* Handles unexpected traps and interrupts.  Because it is difficult to
 * determine which trap or interrupt was taken, it doesn't try.  You
 * just get the "Unexpected Trap" message and end up in the interpreter...
 */
{
	maskbkpts ();
	printf ("Unexpected interrupt/trap; next addr =  ");
	displong (regsave[PC], radix);
	printf ("\n");
	curbkpt = NULL;
	interp ();
}


interp ()

/* The main interpreter loop.  Called at startup, and from trap processing
 * routines.  Reads commands from keyboard and processes them.
 */

{
	register long	val1 = 0;		/* first typed value on line */
	register long	val2 = 0;		/* second typed value */
	register char	c;			/* next char to process */
	register short	val1flag = FALSE;	/* value 1 entered */
	register short	val2flag = FALSE;	/* value 2 entered */
	
	for (;;) {
		c = getchar ();
		if (isnum(c) || ishex(c)) {
			val1 = (val1 * radix) + (c > '9' ? c + 10 - 'A'
							 : c - '0');
			val1flag = TRUE;
			continue;
		} else switch (c) {

			case DEL:	/* rubout - kill line */
				printf ("XXX   ");
				setmodes ();
				break;

			case 'a':	/* address register */
				if ((c = getchar ()) < '0' || c > '7') {
					printf ("???   ");
					break;
				}
				dot = (long)&regsave[ADDR + c - '0'];
				break;

			case 'd':	/* data register */
				if ((c = getchar ()) < '0' || c > '7') {
					printf ("???   ");
					break;
				}
				dot = (long)&regsave[DATA + c - '0'];
				break;

			case 's':	/* sp or sr */
				if ((c = getchar ()) == 'p')
					dot = (long)&regsave[SP];
				else if (c == 'r')
					dot = (long)&regsave[SR];
				else
					printf ("???   ");
				break;

			case 'p':	/* pc */
				if ((c = getchar ()) != 'c') {
					printf ("???   ");
					break;
				}
				dot = (long)&regsave[PC];
				break;

			case '\r':	/* <cr> - modify and close word */
				if (val1flag)
					deposit (val1, dot, size);
				setmodes ();
				break;
				
			case '\n':	/* <lf> - modify, close, open next */
				if (val1flag)
					deposit (val1, dot, size);
				if (mode == INSTR)
					dot += dotinc;
				else
					dot += size;
				displong (dot, radix);
				exam (dot, size, mode, radix);
				break;

			case '^':	 /* ^ - modify, close, open prev */
				if (val1flag)
					deposit (val1, dot, size);
				dot -= size;
				printf ("\n");
				displong (dot, radix);
				exam (dot, size, mode, radix);
				break;

			case TAB:	/* tab - modify, close, open indir. */
				if (val1flag)
					deposit (val1, dot, size);
				dot = *(long *)dot;
				printf ("\n");
				displong (dot, radix);
				exam (dot, size, mode, radix);
				break;

			case '/':	/* / - open in current mode */
				if (val1flag)
					dot = val1;
				exam (dot, size, mode, radix);
				break;

			case '[':	/* [ - open as numeric */
				if (val1flag)
					dot = val1;
				exam (dot, size, NUMERIC, radix);
				break;

			case ']':	/* ] - open as instruction */
				if (val1flag)
					dot = val1;
				exam (dot, size, INSTR, radix);
				break;

			case '!':	/* ! - open but don't display */
				if (val1flag)
					dot = val1;
				break;

			case ';':	/* ; - redisplay current loc */
				exam (dot, size, mode, radix);
				break;

			case '=':	/* = - redisplay as number */
				if (val1flag)
					dot = val1;
				exam (dot, size, NUMERIC, radix);
				break;

			case '_':	/* _ - redisplay as instruction */
				exam (dot, size, INSTR, radix);
				break;

			case ESC:	/* <esc> - prefix for extended cmds */
				c = getchar ();
				if (c == ESC) { /* set modes permanently */
					perm = TRUE;
					c = getchar ();
				}
				while (isnum(c)) {
					val2 = (val2 * 10) + c - '0';
					val2flag = TRUE;
					c = getchar ();
				}
				switch (tolower (c)) {

					case DEL: /* rubout - kill line */
						printf ("XXX   ");
						setmodes ();
						break;

					case 's': /* set instruction mode */
						mode = INSTR;
						if (perm)
							permmode = mode;
						break;

					case 'a':
					case 'c': /* set numeric mode */
						mode = NUMERIC;
						if (perm)
							permmode = mode;
						break;

					case 'r': /* set radix */
						if (val2flag && (val2 == 8 ||
						    val2 == 10 || val2 == 16))
							radix = val2;
						else
							radix = 16;
						if (perm)
							permradix = radix;
						break;

					case 'h': /* set byte mode */
						size = 1;
						if (perm)
							permsize = size;
						break;

					case 'w': /* set word mode */
						size = 2;
						if (perm)
							permsize = size;
						break;

					case 'l': /* set longword mode */
						size = 4;
						if (perm)
							permsize = size;
						break;

					case 'g': /* start program execution */
						if (val1flag)
							exec (val1);
						else
							exec (START);
						break;

					case 'b': /* set/clear bkpt */
						if (val1flag)
							setbkpt(val1,val2flag,
								val2);
						else
							rembkpt(val2flag,val2);
						printf ("\t");
						break;

					case 'p': /* proceed from breakpoint */
						proceed ();
						break;

					case 'u': /* turn single-step on/off */
						c = getchar ();
						if (val2flag)
							sstep = TRUE;
						else
							sstep = FALSE;
						printf ("\t");
						break;

					default:
						printf ("???   ");
						break;
				}
				break;

			default:
				printf ("???   ");
				break;
		}

		val1 = val2 = 0;
		val1flag = val2flag = FALSE;
		perm = FALSE;
	}
}


exam (addr, size, mode, radix)

/* Display the contents of size bytes starting at addr, according
 * to mode.  If mode is NUMERIC, radix contains the display radix.
 *
 * Arguments: */

register long	addr;			/* address for display */
register short	size;			/* word size for display */
register short	mode;			/* display mode */
register short	radix;			/* display radix */
{
	static	char	*fmts[4][5] = {	/* formats for display */
		"", "%c", "%c%c", "", "%c%c%c%c",
		"", "%o", "%o", "", "%O",
		"", "%d", "%d", "", "%D",
		"", "%x", "%x", "", "%X" };
	register char	*fmt;
	
	if (mode == INSTR) {
		printins (radix, 0, *(short *)addr);
		return;
	}
	
	fmt = fmts[(mode == TEXT ? 0 : radix == 8 ? 1 : radix == 10 ? 2 : 3)]
		  [size];

	printf ("	");
	switch (size) {
		case 1:			/* bytes */
			printf (fmt, (*(char *)addr) & 0xFF);
			break;
		case 2:
			if (mode == TEXT)
				printf (fmt, *(char *)addr,
					*(char *)(addr + 1));
			else
				printf (fmt, (*(short *)addr) & 0xFFFF);
			break;
		case 4:
			if (mode == TEXT)
				printf (fmt, *(char *)addr,
					*(char *)(addr+1), *(char *)(addr+2),
					*(char *)(addr + 3));
			else
				printf (fmt, *(long *)addr);
			break;
	}
	printf ("\t");
}


deposit (val, addr, size)

/* Deposit size bytes of the specified value in the specified address
 *
 * Arguments: */

register long	val;			/* value to be deposited */
register long	addr;			/* address to be updated */
register short	size;			/* number of bytes to update */
{
	switch (size) {
		case 1:
			*(char *)addr = (char)val;
			break;
		case 2:
			*(short *)addr = (short)val;
			break;
		case 4:
			*(long *)addr = (long)val;
			break;
	}
}


displong (word, radix)

/* Display the specified longword according to the specified radix.
 *
 * Arguments: */

register long	word;			/* word to display */
register short	radix;			/* radix */
{
	register char	*fmt;
	
	switch (radix) {
		case 8:
			fmt = "%O";
			break;
		case 10:
			fmt = "%D";
			break;
		case 16:
			fmt = "%X";
			break;
	}
	printf (fmt, word);
}


setmodes ()

/* Set all temporarily-modified modes back to their permanent settings */

{
	size = permsize;
	mode = permmode;
	radix = permradix;
}


static getchar ()

/* Get a character from UNIX, and echo it back */

{
	register int	ch;
	
	ch = getc ();
	if (ch == '\n') {
		putc ('\r');
		putc ('\n');
	} else if (ch == '\r') {
		putc ('\r');
		putc ('\n');
	} else if (ch == ESC) {
		putc ('$');
	} else if (ch == '\t') {
		putc (ch);
	} else if (iscntl(ch)) {
		putc ('^');
		putc (ch + 0100);
	} else
		putc (ch);
	return (ch);
}


setbkpt (addr, flag, bkptno)

/* Set breakpoint number bkptno (or the first available breakpoint, if
 * flag is FALSE) at the specified address.
 *
 * Arguments: */

register long	addr;			/* address to set breakpoint at */
register short	flag;			/* TRUE if bkptno specified */
register long	bkptno;			/* breakpoint number if flag=TRUE */
{
	if (!flag)
		for (bkptno = 0; bkptno < NOBKPT; bkptno++)
			if (!bkpt[bkptno].bk_active)
				break;
	if (bkptno >=  NOBKPT) {
		printf ("Too many breakpoints\n");
		return;
	}
	bkpt[bkptno].bk_addr = addr;
	bkpt[bkptno].bk_active = TRUE;
}


rembkpt (flag, bkptno)

/* Remove breakpoint number bkptno (or all breakpoints if flag is FALSE).
 *
 * Arguments: */

register short	flag;			/* TRUE if bkptno is specified */
register long	bkptno;			/* breakpoint no. if flag = TRUE */
{
	if (!flag)
		for (bkptno = 0; bkptno < NOBKPT; bkptno++)
			bkpt[bkptno].bk_active = FALSE;
	else
		bkpt[bkptno].bk_active = FALSE;
}


maskbkpts ()

/* Mask off all currently active breakpoints so the user won't see them
 * when he examines memory, but leave them active so when he returns
 * to the program they will be unmasked.
 */
{
	register int	i;			/* temp index */
	
	for (i = 0; i < NOBKPT; i++)
		if (bkpt[i].bk_active)
			*(short *)(bkpt[i].bk_addr) = bkpt[i].bk_instr;
}


unmaskbkpts ()

/* Unmask all currently active breakpoints so they will be executed.
 * Should be called just before proceeding with user's program.
 */
{
	register int	i;			/* temp. index */
	
	for (i = 0; i < NOBKPT; i++)
		if (bkpt[i].bk_active) {
			bkpt[i].bk_instr = *(short *)(bkpt[i].bk_addr);
			*(short *)(bkpt[i].bk_addr) = BKPTRAP;
		}
}


proceed ()

/* Proceed from breakpoint or single-step.  If not single-stepping,
 * unmask breakpoints so they will be caught; also, need to single-
 * step through one instruction to insure that the instruction
 * displaced by the current breakpoint will be executed.
 */
{
	printf ("\n");
	regsave[SR] &= ~TRACE;
	if (sstep) {
		regsave[SR] |= TRACE;
		cont = FALSE;
	} else {
		unmaskbkpts ();
		if (curbkpt != NULL && curbkpt->bk_addr == regsave[PC] &&
		    curbkpt->bk_active) {
			*(short *)(curbkpt->bk_addr) = curbkpt->bk_instr;
			regsave[SR] |= TRACE;
			cont = TRUE;
		}
	}
	rtn ();
}


exec (addr)

/* Begin execution of user's program at the specified address.
 *
 * Arguments: */

register long	addr;			/* execution address */
{
	
	regsave[PC] = addr;
	regsave[SP] = (long)&tempstack[0]; /* get a temporary stack */
	curbkpt = NULL;
	proceed ();
}
