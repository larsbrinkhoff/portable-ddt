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


/* This file contains all the actual operator execution
 * control routines. All commands (whether $ or operator)
 * come here.
 */


#ifndef	DDTCAT	/* Don't reget hdrs if catting when all routines internal */

#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"

ext	unsw	d_go;		/* Need these if not being compiled together */
ext	unsf	d_ncp;
ext	unst	d_hlt;
ext	char	d_crlf[];

#endif



/* Structure for controlling number of args to $ commands.
 */
		
#define	ddtccb	struct	dccbstr

ddtccb	{	char	dc_cmd;		/* Command character */
		twrd	dc_typ;		/* Whether two $'s legal */
		unst	dc_min;		/* Min and max no of args */
		unst	dc_max;
		};

intern	ddtccb	d_ct[] =	{	{ 'A', 1, 0, 0 },
					{ 'B', 0, 0, 2 },
					{ 'D', 0, 1, 1 },
					{ 'G', 0, 0, 1 },
					{ 'H', 1, 0, 0 },
					{ 'I', 1, 0, 1 },
					{ 'L', 1, 0, 0 },
					{ 'M', 0, MDMIN, MDMAX },
					{ 'N', 1, 0, 0 },
					{ 'O', 1, 1, 1 },
					{ 'P', 0, 0, 1 },
					{ 'R', 1, 0, 1 },
					{ 'S', 1, 0, 1 },
					{ 'T', 1, 0, 0 },
					{ 'V', 0, 0, 0 },
					{ 'W', 1, 0, 0 },
					{ 'Z', 1, 0, 0 },
					{ 0, 0, 0, 0,}
				};


/* Useful info, etc */

ext	word	DEFPC;		/* Default start address */


/* Useful macros for common return method in main operator
 * routines, etc.
 */

#define	eret()		d_err(dsp); return(C_NONE)



/* Main input operator character handler. The code in each case should
 * be fairly self explanatory; see ddt.h for the meaning of the
 * global static flags, etc. Note that all the commands here are
 * only allowed at most one argument, so error detection is done
 * first.
 */

intern	twrd	d_docmd(dsp, c)
reg	ddtst	*dsp;
	char	c;

{	reg	unsf	tmp;
		unst	mdtmp;
		unsw	d_getval();
		unsf	d_getbsz();
		retcd	d_bdsp();
		retcd	d_exam();
		retcd	d_dep();
		retcd	d_fetch();
		PSTYP	md_stop();
	
	if (d_numval(dsp) > 1) {
		eret();
		}

	switch(c) {

	  case '/':	if (d_numval(dsp) != 0)
				dsp->d_dot = d_getval(dsp);
			goto opnloc;

	  case '!':	if (d_numval(dsp) != 0)
				dsp->d_dot = d_getval(dsp);
			d_stdone(dsp, S_OPN);
			(*dsp->d_putc)('\t');
			break;

	  case '\r':	if ((d_numval(dsp) != 0) &&
			    ((dsp->d_oprst != S_OPN) || (d_dep(dsp) == ERR))) {
					eret();
					}
			(*dsp->d_puts)(&d_crlf[0]);
			d_reset(dsp);
			break;

	  case '\n':
	  case '^':	if ((d_numval(dsp) != 0) &&
			    ((dsp->d_oprst != S_OPN) || (d_dep(dsp) == ERR))) {
				eret();
				}
			if (dsp->d_oprst == S_BLK) {
				if (c == '\n')
					dsp->d_dot += dsp->d_isize;
				  else
					dsp->d_dot -= dsp->d_isize;
				d_proloc(dsp, dsp->d_dot);
				if (d_bdsp(dsp, dsp->d_isize) == ERR) {
					eret();
					}
				d_stdone(dsp, S_BLK);
				break;
				}
			if (dsp->d_oprst != S_OPN) {
				eret();
				}
			if (c == '\n')
				dsp->d_dot += dsp->d_isize;
			  else
				dsp->d_dot -= dsp->d_isize;
			(*dsp->d_puts)(&d_crlf[0]);
			d_proloc(dsp, dsp->d_dot);
			goto opnloc;

	  case '@':
	  case '\t':	if (d_numval(dsp) != 0)
				if ((dsp->d_oprst != S_OPN) ||
				    (d_dep(dsp) == ERR)) {
					eret();
					}
			if ((dsp->d_oprst != S_OPN) ||
			    (dsp->d_isize != sizeof(word))) {
				eret();
				}
			if (d_fetch(dsp, A_DATA, sizeof(word), dsp->d_dot,
							&dsp->d_dot) == ERR) {
				eret();
				}
			(*dsp->d_puts)(&d_crlf[0]);
			d_proloc(dsp, dsp->d_dot);
			goto opnloc;

	  case '[':	if (dsp->d_size != sizeof(word))
				c = '{';
	  case ']':
	  case '{':	if (d_numval(dsp) != 0)
				dsp->d_dot = d_getval(dsp);
			  else
				if (dsp->d_oprst != S_OPN) {
					eret();
					}
			dsp->d_valmode = ((c == ']') ? M_INST : M_SYM);
			if (c == '{')
				dsp->d_valmode = M_NUM;
			goto opnloc;

	  case '=':	if (d_numval(dsp) != 0) {
				d_lprrdx(dsp, ((unsl) d_getval(dsp)),
							dsp->d_obase);
				d_stdone(dsp, dsp->d_oprst);
				if (dsp->d_oprst == S_NONE)
					(*dsp->d_puts)(&d_crlf[0]);
				  else
					(*dsp->d_putc)('\t');
				break;
				}
			if (dsp->d_oprst != S_OPN) {
				eret();
				}
			mdtmp = dsp->d_valmode;
			dsp->d_valmode = M_NUM;
			if (d_exam(dsp) == ERR) {
				eret();
				}
			dsp->d_valmode = mdtmp;
			d_stdone(dsp, S_OPN);
			(*dsp->d_putc)('\t');
			break;

	  case '\\':	if (d_numval(dsp) != 0)
				dsp->d_dot = d_getval(dsp);
			if ((tmp = d_getbsz(dsp)) == ERR) {
				eret();
				}
			if (d_bdsp(dsp, tmp) == ERR) {
				eret();
				}
			d_stdone(dsp, S_BLK);
			break;

	  case '':
	  case '':	if (dsp->d_ctxt != C_STOP) {		
				eret();
				}
			if (d_numval(dsp) != 0)
				d_ncp = d_getval(dsp);
			  else
				d_ncp = 1;
			d_done(dsp);
			return((c == '') ? C_SST : C_XST);

	  case '':	if (dsp->d_ctxt != C_RUN) {
				eret();
				}
			(*dsp->d_puts)(&d_crlf[0]);
			d_hlt++;
			md_start(md_stop() | PSTRC);
			break;

	  opnloc:	if (d_exam(dsp) == ERR) {
				eret();
				}
			d_stdone(dsp, S_OPN);
			(*dsp->d_putc)('\t');
			break;

	  default:	eret();

	  }

	return(C_NONE);
}


/* Processes escape command. Notice the way in which the state of the
 * input machine affects the commands. Note that all cases simply
 * break out normally; code at bottom checks to make sure if all
 * arguments have been used and to call d_done() to declare parser
 * state needs resetting (i.e. the command is completely finished).
 */

intern	twrd	d_doesc(dsp, c)
reg	ddtst	*dsp;
reg	char	c;

{	reg	ddtccb	*dcp;
		unsw	tmp;
		retcd	d_setibase();
		retcd	d_setobase();
		retcd	md_mdepmd();

	dcp = &d_ct[0];
	while (dcp->dc_cmd != 0) {
		if (dcp->dc_cmd == c)
			break;
		dcp++;
		}
			
	if ((dcp->dc_cmd == 0) || 
	    ((dsp->d_inpst == S_TESC) && (dcp->dc_typ == 0)) ||
	    (d_numval(dsp) < dcp->dc_min) || (d_numval(dsp) > dcp->dc_max)) {
		eret();
		}
	
	switch(c) {

	  case 'A':	setmode(dsp, M_NUM, d_adrmode, d_permamode);
			break;

	  case 'R':	if (d_numval(dsp) != 0) {
				tmp = d_getval(dsp);
				if (d_setibase(dsp, tmp) == ERR)
					return(C_NONE);
				if (d_setobase(dsp, tmp) == ERR)
					return(C_NONE);
				}
			  else {
				setmode(dsp, M_SYM, d_adrmode, d_permamode);
				}
			break;

	  case 'N':	setmode(dsp, M_NUM, d_valmode, d_permvmode);
			break;

	  case 'S':	if (d_numval(dsp) != 0)
				dsp->d_symoff = d_getval(dsp);
			  else {
				setmode(dsp, M_SYM, d_valmode, d_permvmode);
				}
			break;

	  case 'T':	setmode(dsp, M_TXT, d_valmode, d_permvmode);
			break;

	  case 'Z':	setmode(dsp, M_STR, d_valmode, d_permvmode);
			break;

	  case 'H':	setmode(dsp, sizeof(byte), d_size, d_permsize);
			break;

	  case 'W':	setmode(dsp, sizeof(swrd), d_size, d_permsize);
			break;

	  case 'L':	setmode(dsp, sizeof(lwrd), d_size, d_permsize);
			break;

	  case 'I':	if (d_numval(dsp) != 0) {
				if (d_setibase(dsp, d_getval(dsp)) == ERR)
					return(C_NONE);
				}
			  else {
				setmode(dsp, M_INST, d_valmode, d_permvmode);
				}
			break;
				
	  case 'O':	if (d_setobase(dsp, d_getval(dsp)) == ERR)
				return(C_NONE);
			break;

	  case 'D':	tmp = d_getval(dsp);
			if ((tmp < MINLEN) || (tmp > MAXLEN)) {
				eret();
				}
			dsp->d_disl = tmp;
			break;

	  case 'M':	if (md_mdepmd(dsp) == ERR) {
				eret();
				}
			break;

	  case 'B':	if (d_dobptcmd(dsp) == ERR) {
				eret();
				}
			break;

	  case 'V':	d_lstbpts(dsp);
			break;

	  case 'P':	if (dsp->d_ctxt != C_STOP) {
				eret();
				}
			if (d_numval(dsp) != 0)
				d_ncp = d_getval(dsp);
			  else
				d_ncp = 1;
			d_done(dsp);
			return(C_PROC);

	  case 'G':	if ((dsp->d_ctxt != C_STOP) ||
			    ((d_numeval(dsp) == 0) && (d_numval(dsp) != 0))) {
				eret();
				}
			if (d_numval(dsp) != 0)
				d_go = d_getval(dsp);
			  else
				d_go = ((unsw) &DEFPC);
			d_reset(dsp);
			return(C_STRT);

	  default:	ddtbug("unhandled legal cmd");

	}

	if (d_numval(dsp) != 0)
		ddtbug("unused vals");

	(*dsp->d_puts)(&d_crlf[0]);
	d_done(dsp);
	return(C_NONE);
}


/* Print an address as having been opened.
 */

intern	d_proloc(dsp, loc)
	ddtst	*dsp;
	unsw	loc;

{	d_prloc(dsp, dsp->d_dot);
	(*dsp->d_putc)('/');
}
