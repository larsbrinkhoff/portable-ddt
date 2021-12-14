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


/* Top level for machine independant DDT. See comments in ddt.h
 * All instances of DDT come through here to parse commands;
 * returns only in instances that run with the machine stopped,
 * i.e. instance where machine control commands are accessible.
 *
 * This file contains the routines that make up the bulk of the parser;
 * some simple parsing is done in the operator routines but it is
 * usually just a case statement.
 *
 * The general design of the parser is that there is an input machine
 * with state, as well as an input holding buffer. Due to the obscure DDT
 * syntax, you usually find out that you're in a new section of the command
 * after the input character that caused the change is in your paws;
 * it's usually much easier and cleaner to just put the char back in
 * the input stream and let the proper guy hack it, rather than anything
 * else.
 *
 * When the parser decides it has the end of a command (usually somewhere
 * down inside the call chain, done() is called to reset the parser to
 * accept a new command. Some state, like the size of the last operand,
 * and whether or not there is a location open, is left for the next command
 * which may need to know it. d_err() is called on error, and completely
 * resets the state to what it would have been when DDT was entered
 * using reset() after printing an error message. Things that stay
 * across DDT invocations are the breakpoints, the location counter and
 * the permanent modes.
 */


#ifndef	DDTCAT	/* Don't reget hdrs if catting when all routines internal */
#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"
#endif



/* Top level of parser. It's usually at this level when on a new
 * command, or after one of the special characters that
 * causes state changes has been seen; i.e. between tokens.
 * This level never returns in the context that runs under
 * the normal running code; it can only return in the context
 * that runs when the machine is halted.
 * This routine limits the command set of DDT; note that no symbols
 * may be typed as part of a value after a '$', since such characters
 * cannot be unambiguously separated from '$' commands. Also, it
 * is a fact of the way the input state machine works that no operators
 * can be used there either. This could be fixed but it does not
 * seem useful.
 *
 * 'The reason the parser is in two pieces is the ambiguous use
 * of '.' in symbols and as a symbol.'
 */

twrd	ddtparse(dsp)
reg	ddtst	*dsp;

{	reg	char	c;
	xreg	char	oc;
	reg	twrd	cmd;
		char	d_getc();
		twrd	d_doesc();
		twrd	d_dochar();
		retcd	d_issymch();

	d_reset(dsp);

	for (;;) {
		oc = d_getc(dsp);
		c = ucify(oc);

		if (c == '') {
			(*dsp->d_puts)("\tXXX\n\r");
			d_done(dsp);
			continue;
			}
			
		if ((dsp->d_inpst == S_ESC) || (dsp->d_inpst == S_TESC)) {
			if ((c >= 'A') && (c <= 'Z')) {
				d_echoc(dsp, c);
				if (d_openval(dsp) != 0)
					d_endval(dsp);
				if ((cmd = d_doesc(dsp, c)) != C_NONE) {
					if (dsp->d_ctxt != C_STOP)
						ddtbug("run cmd in run mode");
					  else
						return(cmd);
					}
				continue;
				}
			if (dsp->d_inpst == S_TESC) {
				d_err(dsp);
				continue;
				}
			}

		if (d_issymch(oc) != 0) {
			d_ungetc(dsp, oc);
			if (dsp->d_inpst == S_DOT) {
				dsp->d_inpst = S_NONE;
				d_ungetc(dsp, '.');
				}
			if ((d_openval(dsp) != 0) &&
			    (dsp->d_inpst == S_NONE)) {
				d_err(dsp);
				continue;
				}
			d_rdsym(dsp);
			continue;
			}

		if ((c >= '0') && (c <= '9')) {
			d_ungetc(dsp, oc);
			d_rdnum(dsp);
			continue;
			}

		cmd = d_dochar(dsp, c);
		if (cmd != C_NONE) {
			if (dsp->d_ctxt != C_STOP)
				ddtbug("run cmd in run mode");
			  else
				return(cmd);
			}
		}
}


/* Look at a received character; if it's part of a command pass it
 * on, otherwise hack the state machine and return.
 */

intern	twrd	d_dochar(dsp, c)
reg	ddtst	*dsp;
reg	char	c;

{	reg	twrd	cmd;
		twrd	d_docmd();

	if ((dsp->d_inpst == S_DOT) && (c != '.')) {
		d_echoc(dsp, '.');
		dsp->d_inpst = S_NONE;
		d_setval(dsp, dsp->d_dot);
		}

	switch(c) {

	  case ',':	if ((d_openval(dsp) != 0) &&
			    ((dsp->d_inpst == S_NONE) ||
			     (dsp->d_inpst == S_ESC) ||
			     (dsp->d_inpst == S_TESC))) {
				d_endval(dsp);
				break;
				}
			d_err(dsp);
			return(C_NONE);
		
	  case '':	if (d_openval(dsp) != 0)
				d_endval(dsp);
			if (dsp->d_inpst == S_NONE) {
				d_seteval(dsp);
				dsp->d_inpst = S_ESC;
				break;
				}
			if (dsp->d_inpst == S_ESC) {
				dsp->d_inpst = S_TESC;
				break;
				}
			d_err(dsp);
			return(C_NONE);

	  case '.':	if (dsp->d_inpst == S_NONE) {
				dsp->d_inpst = S_DOT;
				return(C_NONE);
				}
			if (dsp->d_inpst == S_DOT) {
				d_ungetc(dsp, '.');
				d_ungetc(dsp, '.');
				dsp->d_inpst = S_NONE;
				d_rdsym(dsp);
				return(C_NONE);
				}
			d_err(dsp);
			return(C_NONE);

	  default:	if (dsp->d_inpst != S_NONE) {
				d_err(dsp);
				return(C_NONE);
				}
			switch(c) {

			  case ' ':	
			  case '+':	dsp->d_inpst = S_ADD;
					break;

			  case '-':	if (d_openval(dsp) == 0)
						d_setval(dsp, 0);
					dsp->d_inpst = S_SUB;
					break;
					
			  case '*':	dsp->d_inpst = S_MUL;
					break;

			  default:	if (d_openval(dsp) != 0)
						d_endval(dsp);
					d_echoc(dsp, c);
					cmd = d_docmd(dsp, c);
					if (d_numval(dsp) != 0)
						ddtbug("unused vals");
					return(cmd);
			  }

			if (d_openval(dsp) == 0)
				d_err(dsp);
	  }

	d_echoc(dsp, c);
	return(C_NONE);
}


/* Read a number; this routines reads a number from the input stream, and
 * then saves it. Note that numbers after a '$' has been read and not
 * started with a '0' are read in base 10.
 */

intern	d_rdnum(dsp)
reg	ddtst	*dsp;

{	reg	unsw	tmp;
	reg	char	c;
	xreg	char	oc;
	xreg	unst	ibase;

	ibase = dsp->d_ibase;
	if ((dsp->d_inpst == S_ESC) || (dsp->d_inpst == S_TESC)) {
		c = d_getc(dsp);
		if (c != '0')
			ibase = 10;
		d_ungetc(dsp, c);
		}

	tmp = 0;

	for (;;) {
		oc = d_getc(dsp);
		if (oc == '') {
			if (tmp == 0) {
				d_ungetc(dsp, '');
				return;
				}
			(*dsp->d_puts)(" ");
			tmp /= ibase;
			continue;
			}

		c = ucify(oc);
		if ((c >= '0') && (c <= '9')) {
			d_echoc(dsp, c);
			c = (c - '0');
			}
		  else {
			if ((c >= 'A') && (c <= 'F') && (ibase > 10)) {
				d_echoc(dsp, c);
				c = (c + 10 - 'A');
				}
			  else {
				d_ungetc(dsp, oc);
				break;
				}
			}

		if (c < ibase) {
			tmp *= ibase;
			tmp += c;
			continue;
			}
		  else {
			d_err(dsp);
			return;
			}
		}

	d_setval(dsp, tmp);
}
