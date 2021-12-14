#

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


/* This file contains the routines for handling the symbol
 * table. This version is for the RADIX-50 symbol table.
 */


#ifndef	DDTCAT	/* Don't reget hdrs if catting when all routines internal */
#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"
#endif



/* Info for ASCII to RADIX-50 conversion */

#define RAD	40		/* Radix base */
#define NCS	3		/* No of chars/shortword */

#define	UCS	0101		/* Upper case */
#define	UCE	0132
#define	UCD	0100
#define	NOS	060		/* Numbers */
#define NOE	071
#define	NOD	022
#define	NUL	00
#define	DOL	033		/* Specials  $ and . */
#define	DOT	034
#define	RAND	035		/* Everything else gets this */


/* DDT symbol table entry format */

#define	SYMSIZ	6		/* Size of RADIX50 symbol in ASCII */
#define	NOSYM	0		/* Both words this mean end of table */

#define ddtsym struct dsestr

ddtsym	{	unss	d_sym[(SYMSIZ / NCS)];
		unsw	d_val;
	};


/* Built in symbol tables; first is only useful in hardcore debugger
 * when it's not running, since it records the location of registers
 * when stored internally. It MUST immediately prepend the normal
 * symbol table.
 */

ext	ddtsym	D_ISYMT[];
ext	ddtsym	D_SYMT[];


/* Radix-50 character table */

intern	char	d_rtbl[] =	" ABCDEFGHIJKLMNOPQRSTUVWXYZ$._0123456789";



/* Reads a symbol name off the input stream, allowing only legal characters,
 * and attempts to find it in the symbol table. Converts the name
 * to RADIX-50 format first, since this is the form that characters
 * in the symbol table are saved in. Note slight non-portability at
 * end where (SYMSIZ / NCS) is known to be 2; used to speed up table lookup.
 * It's not worth tryng to split out the start of this routine into the
 * parser; it's too dependent on the symbol table organization.
 *
 * Note that the ISYMT is only used when the machine has stopped. This
 * is because ISYMT (which MUST be immediately before DSYMT for this
 * symbol table implementation contain symbols which reference the
 * internal locations in the debugger assembly toehold where the
 * registers are stored when the machine is not running.
 *
 * Doesn't bother to use buffer in DDTST since stack has lots of room
 * in input stage.
 */

intern	d_rdsym(dsp)
xreg	ddtst	*dsp;

{	reg	ddtsym	*sym;
	reg	char	c;
	xreg	char	oc;
	reg	unst	p;
	xreg	unss	sum;
	xreg	unsw	i;
	xreg	unst	j;
		char	buf[SYMSIZ];
		unsw	symv[(SYMSIZ / NCS)];

	p = 0;
	for (;;) {
		oc = d_getc(dsp);
		if (oc == '') {
			if (p <= 0) {
				d_ungetc(dsp, '');
				return;
				}
			(*dsp->d_puts)(" ");
			p--;
			continue;
			}

		c = ucify(oc);
		if (((c >= 'A') && (c <= 'Z')) || ((c >= '0') && (c <= '9')) ||
		    (c == '$') || (c == '.') || (c == '_')) {
			if (p < SYMSIZ) {
				d_echoc(dsp, c);
				buf[p++] = c;
				}
			continue;
			}
		d_ungetc(dsp, c);
		break;
		}

	while (p < SYMSIZ)
		buf[p++] = 0;
	p = 0;

	for (i = 0; i <	(SYMSIZ / NCS); i++) {
		sum = 0;
		for (j = 0; j < NCS; j++) {
			sum =* RAD;
			c = buf[p++];
			if ((c >= UCS) && (c <= UCE))
				c =- UCD;
			  else if ((c >= NOS) && (c <= NOE))
				c =- NOD;
			  else if (c == NUL)
				c = NUL;
			  else if (c == '$')
				c = DOL;
			  else if (c == '.')
				c = DOT;
			  else if (c == '_')
			  	c = RAND;
			  else
				ddtbug("conversion err");
			sum =+ c;
			}
		symv[i] = sum;
		}

	if (dsp->d_ctxt == C_STOP)
		sym = &D_ISYMT[0];
	  else
		sym = &D_SYMT[0];

	for (; ((sym->d_sym[0] != NOSYM) || (sym->d_sym[1] != NOSYM)); sym++)
		if ((sym->d_sym[0] == symv[0]) && (sym->d_sym[1] == symv[1]))
			break;

	if ((sym->d_sym[0] == NOSYM) && (sym->d_sym[1] == NOSYM)) {
		d_err(dsp);
		return;
		}
	
	d_setval(dsp, sym->d_val);
}


/* Given a value, print it in symbolic (name+offset) form. Note that
 * the offset is printed in the input base; this is because it is assumed
 * that this form is more useful. Once again, note that size of symbol
 * in shorts is known. Also, note that the general output buffer is
 * used. OBSIZ is greater than SYMSIZ so we are OK.
 */

intern	d_prsym(dsp, loc)
xreg	ddtst	*dsp;
reg	unsw	loc;

{	reg	ddtsym	*sym;
	xreg	ddtsym	*svsym;
	reg	unsw	mindif;
	xreg	unst	symlen;
		unst	d_cnvrdx();

	svsym = NULL;
	if (dsp->d_ctxt == C_STOP)
		sym = &D_ISYMT[0];
	  else
		sym = &D_SYMT[0];


	if (sym != NULL)
		for (mindif = dsp->d_symoff; ((sym->d_sym[0] != NOSYM) ||
					(sym->d_sym[1] != NOSYM)); sym++) {
			if (sym->d_val > loc)
				continue;
			if ((loc - sym->d_val) >= mindif)
				continue;
			mindif = (loc - sym->d_val);
			svsym = sym;
			}

	if ((svsym == NULL) || (svsym->d_val < SYMSML)) {
		d_lprrdx(dsp, ((unsl) loc), dsp->d_ibase);
		return;
		}

	symlen = d_cnvrdx(&svsym->d_sym[0], &dsp->d_opbuf[0]);
	dsp->d_opbuf[symlen] = '\0';
	(*dsp->d_puts)(&dsp->d_opbuf[0]);

	if (mindif) {
		(*dsp->d_putc)('+');
		d_lprrdx(dsp, ((unsl) mindif), dsp->d_ibase);
		}
}


/* Convert a symbol stored as a RADIX-50 characters into ASCII.
 * Returns the length of the name in characters.
 */

intern	unst	d_cnvrdx(in, buf)
xreg	unss	*in;
reg	char	*buf;

{	reg	unss	tmp;
	reg	unss	wd;
	xreg	unst 	j;
	xreg	char	*svbuf;

	svbuf = buf;

	for (j = 0; (j < (SYMSIZ / NCS)); j++) {
		wd = *in++;
		buf[2] = d_rtbl[wd%RAD];
		tmp = wd/RAD;
		buf[1] = d_rtbl[tmp%RAD];
		*buf++ = d_rtbl[tmp/RAD];
		buf++; buf++;
		};

	buf = svbuf;
	for (tmp = 0; tmp < SYMSIZ; tmp++, buf++)
		if (*buf == ' ')
			break;
	return(tmp);
}


/* Is character legal in symbol? Returns non-zero if true, zero
 * if false. Note: parse will croak if '.' is declared legal.
 * Mucho special case code for '.'; note that it currently de facto
 * asssumes that '.' is legal in a symbol. Dunno if this will
 * break things. Assumes NUL is not a legal character!
 */

intern	retcd	d_issymch(c)
reg	char	c;

{	c = ucify(c);
	if (((c >= 'A') && (c <= 'Z')) || (c == '$') || (c == '_'))
		return(c);
	 else
		return(ERR);
}


/* Do we have a symbol table? Returns true if so.
 */

intern	retcd	d_isst()

{	if (&D_SYMT[0] != NULL)
		return(OK);
	  else
		return(ERR);

	if ((SYMSIZ + 1) > OBSIZ)
		ddtbug("buff too small for sym");
}
