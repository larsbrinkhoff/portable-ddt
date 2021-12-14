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


/* This file contains all the small utility routines.
 */


#ifndef	DDTCAT	/* Don't reget hdrs if catting when all routines internal */

#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"

ext	char	d_crlf[];		/* Need to get this address too. */

#endif


/* Control character conversion values */

#define	CTLMIN	040
#define	CTLCVT	0100
#define	CHHIBT	0200



/* Main initialization; called on startup. Sets various
 * default values for permanent modes; calls reset to
 * set temporaries.
 */

ddtinit(dsp)
reg	ddtst	*dsp;

{	retcd	d_isst();

 	dsp->d_permsize = DEFSIZ;
	dsp->d_permvmode = DEFVMOD;
	dsp->d_permamode = DEFAMOD;
	dsp->d_permobase = DEFIBSE;
	dsp->d_permibase = DEFOBSE;

	d_reset(dsp);

	dsp->d_dot = NULL;
	dsp->d_disl = (SCRSIZ * SCRLEN);
	dsp->d_symoff = SYMLG;
	
	(*dsp->d_puts)(&d_crlf[0]);
	if (d_isst() == ERR)
		(*dsp->d_puts)("No Symbol Table\n\r");
}


/* Check and return number of entries to list in block display.
 */

intern	unsf	d_getbsz(dsp)
reg	ddtst	*dsp;

{	reg	unsf	tmp;

	if ((dsp->d_valmode != M_NUM) && (dsp->d_valmode != M_SYM))
		return(ERR);
	tmp = (dsp->d_size * dsp->d_disl);
	if (tmp > MAXLEN)
		tmp = (MAXLEN / dsp->d_size);
	return(tmp);
}


/* Set input base.
 */

intern	retcd	d_setibase(dsp, tmp)
reg	ddtst	*dsp;
reg	unsw	tmp;

{	if ((tmp > MAXBASE) || (tmp < MINBASE)) {
		d_err(dsp);
		return(ERR);
		}
	setmode(dsp, tmp, d_ibase, d_permibase);
	return(OK);
}

/* Set output base.
 */

intern	retcd	d_setobase(dsp, tmp)
reg	ddtst	*dsp;
reg	unsw	tmp;

{	if ((tmp > MAXBASE) || (tmp < MINBASE)) {
		d_err(dsp);
		return(ERR);
		}
	setmode(dsp, tmp, d_obase, d_permobase);
	return(OK);
}


/* Reset all modes to permanent.
 */

intern	d_reset(dsp)
reg	ddtst	*dsp;

{	d_done(dsp);

	dsp->d_size =  dsp->d_permsize;
	dsp->d_adrmode = dsp->d_permamode;
	dsp->d_valmode = dsp->d_permvmode;
	dsp->d_obase = dsp->d_permobase;
	dsp->d_ibase = dsp->d_permibase;
}


/* Reset state of input machine. Since this is called
 * after a command has completed and no location is
 * left open, the location open flag is cleared.
 */

intern	d_done(dsp)
	ddtst	*dsp;

{	d_stdone(dsp, S_NONE);
}


/* Reset state of input machine, and set operator state flag.
 */

intern	d_stdone(dsp, state)
reg	ddtst	*dsp;
	byte	state;

{	dsp->d_inpst = S_NONE;
	dsp->d_oprst = state;
	dsp->d_svccnt = 0;
	dsp->d_svalf = 0;
	dsp->d_svale = 0;
	dsp->d_svalo = 0;
}


/* Get a character from the (possibly buffered) input stream.
 * Note that the user is responsible for echoing the character
 * when he finally decides to use it.
 */

intern	char	d_getc(dsp)
reg	ddtst	*dsp;

{	reg	char	c;

	if (dsp->d_svccnt != 0)
		c = dsp->d_svchr[--dsp->d_svccnt];
	  else {
		c = (*dsp->d_getc)();
		c &= ~CHHIBT;
		}

	return(c);
}


/* Put input character back into buffer.
 */

intern	d_ungetc(dsp, c)
reg	ddtst	*dsp;
char	c;

{	if (dsp->d_svccnt >= IBSIZ)
		ddtbug("char sv buf ovflo");
	dsp->d_svchr[dsp->d_svccnt++] = c;
}


/* Input character echoer.
 */

intern	d_echoc(dsp, c)
reg	ddtst	*dsp;
reg	char	c;

{	if (c == '\n') {
		return;
		}
	else if (c == '\r') {
		return;
		}
	else if (c == '\t') {
		return;
		}
	else if (c == '') {
		(*dsp->d_putc)('$');
		return;
		}
	else d_prc(dsp, c);
}


/* Character printer - prints all characters in legible form.
 */

intern	d_prc(dsp, c)
reg	ddtst	*dsp;
reg	char	c;

{	if ((c & CHHIBT) != 0) {
		(*dsp->d_putc)('');
		c &= ~CHHIBT;
		}

	if (c < CTLMIN) {
		(*dsp->d_putc)('^');
		(*dsp->d_putc)(c + CTLCVT);
		return;
		}
	else if (c == '') {
		(*dsp->d_putc)('^');
		(*dsp->d_putc)('?');
		return;
		}
	else
		(*dsp->d_putc)(c);
}


/* Print the specified number in the specified radix. Achtung!
 * The number argument must be long, not word, to allow for
 * printing longs on 16-bit machines!
 */

intern	d_lprrdx(dsp, no, rdx)
	ddtst	*dsp;
xreg	unsl	no;
reg	unst	rdx;

{	reg	char	*p;
	reg	unsf	tmp;
		char	*bstrt;

	bstrt = &dsp->d_opbuf[0];
	p = (bstrt + OBSIZ);
	*--p = '\0';

	do {	if (p <= bstrt) {
			d_err(dsp);
			return;
			}
		tmp = (no % rdx);
		if (tmp <= 9)
			*--p = (tmp + '0');
		  else
			*--p = (tmp + ('A' - 10));
		no = (no / rdx);
		} while (no != 0);

	(*dsp->d_puts)(p);
}


/* Print the only error message and reset the state completely.
 */

intern	d_err(dsp)
reg	ddtst	*dsp;

{	(*dsp->d_puts)("\t???\n\r");
	d_reset(dsp);
}
