/* EMACS_MODES: c !fill */

/* ddt68.h
 *
 * Parameter definitions for MC68000 mini-debugger.
 *
 */

#define	FALSE		0
#define	TRUE		1
#define	NULL		0

#define	UNIX		0		/* UNIX PCI port */

#define	INSTR		0		/* instruction display mode */
#define	NUMERIC		1		/* numeric display mode */
#define	TEXT		2		/* text display mode */

#define	NOBKPT		8		/* number of breakpoints */

#define	TAB		011		/* tab character */
#define	ESC		033		/* escape character */
#define	DEL		0177		/* rubout character */

/* Register save locations */

#define	DATA		0		/* data registers first */
#define	ADDR		8		/* next address registers */
#define	SP		ADDR + 7	/* sp = a7 */
#define	SR		16		/* next saved sr */
#define	PC		17		/* finally saved pc */

#define	TRACE		0x8000		/* trace bit in SR */
#define	SUPER		0x2000		/* supervisor mode bit */

#define	BKPTRAP		0x4E4F		/* trap #15 - the breakpoint trap */

/* Some useful macros... */

#define	isnum(x)	((x) >= '0' && (x) <= '9')
#define	ishex(x)	((x) >= 'A' && (x) <= 'F')
#define	iscntl(x)	((x) <= 040)
#define	isupper(x)	((x) >= 'A' && (x) <= 'Z')
#define	tolower(x)	(isupper((x)) ? (x) + 'a' - 'A' : (x))
