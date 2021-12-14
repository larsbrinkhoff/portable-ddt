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


/* This software package implements a machine portable DDT.
 * It includes most common DDT capabilites including processor
 * control, symbol tables, etc. It does not (and probably never will)
 * support instruction type-in or $X.
 *
 * It has the capability to support multiple instances of the
 * command parser; this allows you acceess to almost all the
 * DDT capabilites while your software is running, provided that
 * you are prepared to provide an interface to get and send
 * characters. It also needs a stack; i.e. if the 'get-character'
 * routine has no character it must wait in the call chain.
 * Some commands (procesor control) are not accesible from
 * instances that are running under operating software (as opposed
 * to with the machine halted).
 *
 * It comes in two parts; a machine independant core (written in C,
 * which should need NO changes), and some machine dependant 
 * code to handle instruction decoding, as well as machine 
 * dependant code for things like traps. The symbol table managment
 * is also an independant file, so that different symbol table formats
 * can be used.
 *
 * To get it running on the machine of your choice you have to
 * provide a xxtypes.h file (which provides the correct types
 * for your architecture), as well as a xxmdep.h file (which
 * includes information such as the size and value of the
 * breakpoint instruction). You also need a 'instruction decode'
 * routine; this should be a stub if you don't feel like writing
 * one. Also, to use this code for actually controlling the machine
 * (as opposed for using it as a run-time examination tool) you
 * need som machine-language support. See 11instpr.c and 11mdep.mac
 * for an example (PDP11).
 *
 * It should be possible to use the machine portable part (with the
 * instruction decoder) to front-end a remote debugger, but this has
 * not yet been tried. It would need a new symbol table module (since
 * the current ones assume that they are loaded in with the symbol
 * table of the debuggee).
 *
 * As a convenience for reducing the number of symbols, it can be catted
 * together and compiled as one huge file. There is a separate switch
 * to turn off all the global symbols if this is done. If you want
 * to leave them on (for some reason), all the ones which will become
 * visible start with d_. Routines which are machine dependant but
 * could become invisible (i.e. could be in C and compiled together
 * with this) start with md_. Names which will always be visible
 * start with ddt.
 */



/* Make all internal names invisible outside package when debugged;
 * files have to be compiled together (i.e. catted together) for
 * this to work.
 */

#ifdef	DDTNIN			/* On when compiling all files together */
#ifndef	DDTCAT
ERROR;				/* No internal names but files not catted */
#endif
#define	intern	static
#else
#define	intern
#endif



/* Random constants, also type for return code */

#define	NULL	0
#define	ERR	0
#define	OK	1

#define	retcd	unsf



/* Exec context types, continuation codes, states for state machines,
 * modes for typeout, memory access codes, etc.
 */

#define	C_RUN	1		/* Execution context type; system level */
#define	C_STOP	2		/* In hardcore debugger, system stopped */

#define	C_NONE	0		/* Continuation action; none */
#define	C_PROC	1		/* Proceed processor */
#define	C_SST	2		/* Single step processor */
#define	C_XST	3		/* Execute step processor */
#define	C_STRT	4		/* Start processor */


#define	S_NONE	0		/* General state machine idle */

#define	S_ESC	1		/* Input machine state codes */
#define	S_TESC	2		/* Saw escape, double escape */
#define	S_DOT	3		/* Dot, or arithmetic operator */
#define	S_ADD	4
#define	S_SUB	5
#define	S_MUL	6

#define	S_OPN	1		/* Operation machine state codes */
#define	S_BLK	2		/* Location left open, and block exam mode */


#define	M_INST	0		/* Type out mode, address mode */
#define	M_SYM	1		/* Instruction, symbolic, numeric, text */
#define	M_NUM	2
#define	M_TXT	3
#define	M_STR	4


#define	A_DBGI	0		/* Debugger hacking instructions */
#define	A_INST	1		/* Doing some sort of instruction access */
#define	A_DATA	2		/* Looking at data */



/* Constants, parameters. First group fundamental.
 */

#define	MINBASE	2		/* Smallest base that can be handled */
#define	MAXBASE	16		/* Largest base */

#define SCRSIZ	5		/* Max no of values to be printed on a line */



/* State block - contains input machine states, etc. One exists for
 * each mode, under normal operation and with the machine in the
 * hardcore debugger.
 */

#define ddtst struct dststr

ddtst	{	char	(*d_getc)();	/* Input routine for current mode */
		void	(*d_putc)();	/* Output routines */
		void	(*d_puts)();

		twrd	d_ctxt;		/* Context type; normal or stopped */

		twrd	d_inpst;	/* State of input machine */
		twrd	d_oprst;	/* State of operation machine */

		char	d_svchr[IBSIZ];	/* Actual input buffer */
		unst	d_svccnt;	/* Chars saved in input buf */
		unsw	d_svalv[VALSIZ];/* Values typed in */
		unst	d_svalf;	/* Flag for how many values typed */
		unst	d_svale;	/* Number typed before escape */
		unst	d_svalo;	/* Flag if value still open */

		unst	d_isize;	/* Incremental size */
		unsw	d_dot;		/* Current examination location */

		unst	d_size;		/* Data object size, temporary */
		unst	d_permsize;	/* and permanent */
		twrd	d_adrmode;	/* Output modes	*/
		twrd	d_permamode;
		twrd	d_valmode;
		twrd	d_permvmode;
		unst	d_ibase;	/* Bases, input and output */
		unst	d_permibase;
		unst	d_obase;
		unst	d_permobase;
		unsf	d_disl;
		unsf	d_symoff;	/* Max area after sym named with it */

		char	d_opbuf[OBSIZ];	/* Output buffer */

		char	d_char;		/* Memory temporaries */
		byte	d_byte;
		swrd	d_swrd;
		lwrd	d_lwrd;
		word	d_word;

		word	d_mdep[MSIZ];	/* Machine dependant info */
		};




/* Turns fatal DDT error messages into calls to ddt error routine.
 * Saves some space.
 */

#ifndef	DDTDBG
#define	ddtbug(errmsg)		ddterr()
#endif


/* Macros; second ones are part of value package. They look like
 * subroutine calls, which is why they have the leading d_.
 */

#define	ucify(c)	(((c >= 'a') && (c <= 'z')) ? (c - ('a' - 'A')) : c)

#define	d_numval(dsp)	((dsp)->d_svalf)
#define	d_numeval(dsp)	((dsp)->d_svale)
#define	d_openval(dsp)	((dsp)->d_svalo)

#define	setmode(dsp, mode, tloc, ploc)	\
	dsp->tloc = mode; if (dsp->d_inpst == S_TESC) dsp->ploc = dsp->tloc
