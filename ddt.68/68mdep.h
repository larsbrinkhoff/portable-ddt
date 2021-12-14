/*  Copyright 1984 J. Noel Chiappa
 *
 * Permission to use, copy, modify, and distribute this program
 * for any purpose and without fee is hereby granted, provided
 * that this copyright and permission notice appear on all copies
 * and supporting documentation, that my name not be used
 * in advertising or publicity pertaining to distribution of the
 * program without specific prior permission, and notice be given
 * in supporting documentation that copying and distribution is
 * by permission of J. Noel Chiappa. I make no representations about
 * the suitability of this software for any purpose. It is pro-
 * vided "as is" without express or implied warranty.
 */


/* This file contains definitions that can be per-CPU type. The first
 * batch are more parameters than anything fundamental; the second
 * are more machine dependant.
 */

#define MAXSTR	64		/* Max no of bytes of string to print */
#define	SYMSML	0200		/* Ignore symbols with value less than this */
#define	SYMLG	0xffff		/* Initial maximum symbol offset */

#define	SCRLEN	3		/* Initial no of lines for area exam */
#define MINLEN	2		/* Min no of bytes to print in an area exam */
#define MAXLEN	128		/* Max no of bytes */

#define	BPTSIZ	8		/* Number of breakpoints */


#define	IBSIZ	16		/* Size of temporary input buffer */
#define	OBSIZ	32		/* Maximum size of digit, etc to print */
#define	VALSIZ	2		/* Size of value storage area */

#define	MSIZ	1		/* Size of machine dep info */
#define	MDMIN	2		/* Minimum number of machine dependant args */
#define	MDMAX	2		/* Maximum number of machine dependant args */

#define	DEFSIZ	sizeof(word)	/* Default object size */
#define	DEFVMOD	M_INST		/* Default dsiplay mode */
#define	DEFAMOD	M_SYM		/* Default address mode */
#define	DEFIBSE	16		/* Default input and output base */
#define	DEFOBSE	16


#define	MCHSTR	"68000"

#define	BPTTYP	swrd		/* Size of breakpoint instruction */
#define	BPTINST	0x4e4f		/* BPT instruction itself */
#define	BPTOFF	(sizeof(swrd))	/* Amount to backup PC to get to BPT */
#define	XSNBPT	2		/* Number of bpts to place after XST inst */

#define	PSTYP	word		/* Size of PS */
#define	PSNOINT	0x2700		/* PS for no interrupts */
#define	PSTRC	0x8000		/* Trace bit in PS */
#define	DEFPS	PSNOINT		/* Default PS for $G */

#define	PCTYP	word		/* Size of PC */
#define	DEFPC	START_		/* Default start address for $G */
