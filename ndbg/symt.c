/* This file contains the routines for handling the symbol
 * table. This version is for the RADIX-50 symbol table.
 */


#ifndef	DDTCAT	/* Don't reget hdrs if catting when all routines internal */
#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"
#endif


/* DDT symbol table entry format */
#define	SSIZ	32		/* max length of symbols */
#define	NOSYM	0		/* Both words this means end of table */

#define ddtsym struct	dsestr

ddtsym	{	char	*s_name;
		unsw	s_value;
	};

/* Symbol Tables */
ext ddtsym	D_SYMT[];	/* Symbol table */
ext ddtsym	D_ISYMT[];	/* Internal symbol table for registers */

/* Reads a symbol name off the input stream, allowing only legal characters,
 * and attempts to find it in the symbol table. 
 */

intern	d_rdsym(dsp)
xreg	ddtst	*dsp;

{
	reg	ddtsym	*sym;
	reg	int	p;
		unsw	val;
		char	c;
		char	buf[SSIZ];

	p = 0;
	for (;;) {
		c = d_getc(dsp);
		if (c == '') {
			if (p <= 0) {
				d_ungetc(dsp, '');
				return;
				}
			(*dsp->d_puts)(" ");
			p--;
			continue;
			}

		if ( ((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z'))
		    || ((c >= '0') && (c <= '9'))
		    || (c == '$') || (c == '.') || (c == '_')) {
			if (p < SSIZ) {
				d_echoc(dsp, c);
				buf[p++] = c;
			}
			continue;
		}
		d_ungetc(dsp, c);
		break;
	}
	
	buf[p] = '\0';
	
	if (dsp->d_ctxt == C_STOP) sym = D_ISYMT;
	else
		sym = D_SYMT;
	for (; sym->s_name != NOSYM; sym++)
	if (strcmp(sym->s_name, buf) == 0) {
		val = sym->s_value;
		break;
	}

	if (sym->s_name == NOSYM) {
		d_err(dsp);
		return;
	}
	d_setval(dsp, val);
}

/* Given a value, print it in symbolic (name+offset) form.  Note that
 * the offset is printed in the input base; this is because it is assumed
 * that this form is more useful.
 */

intern	d_prsym(dsp, loc)
xreg	ddtst	*dsp;
reg	unsw	loc;

{
	reg	ddtsym	*sym;
	xreg	ddtsym	*svsym;
	reg	unsw	mindif;

	svsym = NULL;

	if (dsp->d_ctxt == C_STOP) sym = &D_ISYMT[0];
	else
		sym = &D_SYMT[0];

	if (sym != NULL)
		for (mindif = dsp->d_symoff; (sym->s_name != NOSYM); sym++) {
			if (sym->s_value > loc)
				continue;
			if ((loc - sym->s_value) >= mindif)
				continue;
			mindif = (loc - sym->s_value);
			svsym = sym;
			}

	if (svsym == NULL) {
		d_lprrdx(dsp, ((unsl) loc), dsp->d_ibase);
		return;
		}

	(*dsp->d_puts)(svsym->s_name);
	if (mindif) {
		(*dsp->d_putc)('+');
		d_lprrdx(dsp, ((unsl) mindif), dsp->d_ibase);
		}
}


/* Is character legal in symbol? Returns non-zero if true, zero
 * if false.
 */

intern	retcd	d_issymch(c)
reg	char	c;

{
	if (((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) ||
	     (c == '$') || (c == '_'))
		return(c);
	 else
		return(ERR);
}


/* Do we have a symbol table? Returns true if so.
 */

intern	retcd	d_isst()

{/*	if (&D_SYMT[0] != NULL)
		return(OK);
	  else */
		return(OK /*ERR*/);
}

