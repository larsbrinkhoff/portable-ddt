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


/* This file contains machine independent operator action routines.
 */


#ifndef	DDTCAT	/* Don't reget hdrs if catting when all routines internal */

#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"

ext	char	d_crlf[];	/* Need to get this address too. */

#endif



/* This routine is called for block display. It takes as an argument
 * the size of the display block in bytes.
 */

intern 	retcd	d_bdsp(dsp, tmp)
reg	ddtst	*dsp;
	unsf	tmp;

{	xreg	unsw	odot;
	reg	unsw	edot;
	reg	unsf	i;
		retcd	d_exam();

	odot = dsp->d_dot;
	edot = (odot + tmp);
	for (i = 0; dsp->d_dot < edot; dsp->d_dot += dsp->d_size) {
		if (d_exam(dsp) == ERR)
			return(ERR);
		if (++i >= SCRSIZ)
			(*dsp->d_puts)(&d_crlf[(i = 0)]);
		}
	(*dsp->d_puts)(&d_crlf[0]);
	dsp->d_isize = (dsp->d_dot - odot);
	dsp->d_dot = odot;

	return(OK);
}


/* Examiner; prints the contents of the current location in the current
 * mode; does checks.
 */

intern	retcd	d_exam(dsp)
reg	ddtst	*dsp;

{	reg	unsw	bloc;
	reg	unsw	tmp;
	xreg	unsl	val;
		retcd	d_fetch();
		unst	md_prinst();
	
	(*dsp->d_putc)('\t');
	switch (dsp->d_valmode) {

	  case M_INST:	if ((tmp = md_prinst(dsp, dsp->d_dot)) == ERR)
				return(ERR);
			dsp->d_isize = tmp;
			break;
			
	  case M_NUM:	tmp = dsp->d_size;
			switch (tmp) {
			  case sizeof(byte):	bloc = ((unsw) &dsp->d_byte);
						break;
			  case sizeof(swrd):	bloc = ((unsw) &dsp->d_swrd);
						break;
			  case sizeof(lwrd):	bloc = ((unsw) &dsp->d_lwrd);
						break;
			  default:		ddtbug("size err in exam");
			  }

			if (d_fetch(dsp, A_DATA, tmp, dsp->d_dot, bloc) == ERR)
				return(ERR);

			switch (tmp) {
			  case sizeof(byte):	val = dsp->d_byte;
/* Fucking PDP11's */				val &= 0377;
						break;
			  case sizeof(swrd):	val = dsp->d_swrd;
						break;
			  case sizeof(lwrd):	val = dsp->d_lwrd;
						break;
			  }

			d_lprrdx(dsp, val, dsp->d_obase);
			dsp->d_isize = tmp;
			break;

	  case M_SYM:	if (dsp->d_size != sizeof(word))
				return(ERR);
			if (d_fetch(dsp, A_DATA, sizeof(word), dsp->d_dot,
							&dsp->d_word) == ERR)
				return(ERR);
			d_prsym(dsp, dsp->d_word);
			dsp->d_isize = sizeof(word);
			break;

	  case M_TXT:	bloc = (dsp->d_dot + dsp->d_size);
			for (tmp = dsp->d_dot; tmp < bloc;
							tmp += sizeof(char)) {
				if (d_fetch(dsp, A_DATA, sizeof(char), tmp,
							 &dsp->d_char) == ERR)
					return(ERR);
				d_prc(dsp, dsp->d_char);
				}
			dsp->d_isize = dsp->d_size;
			break;

	  case M_STR:	bloc = (dsp->d_dot + MAXSTR);
			for (tmp = dsp->d_dot;  tmp < bloc;
							tmp += sizeof(char)) {
				if (d_fetch(dsp, A_DATA, sizeof(char), tmp,
							 &dsp->d_char) == ERR)
					return(ERR);
				if (dsp->d_char == '\0') {
					tmp += sizeof(char);
					break;
					}
				d_prc(dsp, dsp->d_char);
				}
			dsp->d_isize = (tmp - dsp->d_dot);
			break;

	  default:	ddtbug("bad type in exam");
	  }

	return(OK);
}


/* Depositor; makes checks and deposits based on the current size.
 * Value is gotten from the input machine.
 */

intern	retcd	d_dep(dsp)
reg	ddtst	*dsp;

{	reg	word	bloc;
	reg	unsw	sz;
		retcd	d_store();

	sz = dsp->d_size;
	switch (sz) {

	  case(sizeof(byte)):	bloc = ((unsw) &dsp->d_byte);
				dsp->d_byte = d_getval(dsp);
				break;

	  case(sizeof(swrd)):	bloc = ((unsw) &dsp->d_swrd);
				dsp->d_swrd = d_getval(dsp);
				break;

	  case(sizeof(lwrd)):	bloc = ((unsw) &dsp->d_lwrd);
				dsp->d_lwrd = d_getval(dsp);
				break;

	  default:		ddtbug("bad size in dep");
	  }

	dsp->d_isize = sz;
	return(d_store(dsp, A_DATA, sz, bloc, dsp->d_dot));
}


/* Print an address in the current mode.
 */

intern	d_prloc(dsp, loc)
reg	ddtst	*dsp;
	unsw	loc;

{	switch(dsp->d_adrmode) {

	  case M_SYM:	d_prsym(dsp, loc);
			break;

	  case M_NUM:	d_lprrdx(dsp, ((unsl) loc), dsp->d_ibase);
			break;

	  default:	ddtbug("bad mode for adrmode");
	}
}


/* Machine independant fetch and store routines. Basically,
 * checks to see if BPT's need to be masked, then
 * does the appropriate machine dependant operation.
 */

intern	retcd	d_fetch(dsp, mode, sz, src, dst)
reg	ddtst	*dsp;
	twrd	mode;
	unst	sz;
	unsw	src;
	unsw	dst;

{	reg	PSTYP	tmp;
	reg	retcd	val;
		retcd	md_okfetch(), md_fetch();
		PSTYP	md_stop();

	if (md_okfetch(dsp, mode) == ERR)
		return(ERR);

	if ((mode != A_DBGI) && (d_isbpt(src, sz) == ERR)) {
		if (dsp->d_ctxt == C_RUN)
			tmp = md_stop();
		d_mskbpts(dsp);
		val = md_fetch(dsp, mode, sz, src, dst);
		d_unmskbpts(dsp);
		if (dsp->d_ctxt == C_RUN)
			md_start(tmp);
		return(val);
		}

	return(md_fetch(dsp, mode, sz, src, dst));
}

intern	retcd	d_store(dsp, mode, sz, src, dst)
reg	ddtst	*dsp;
	twrd	mode;
	unst	sz;
	unsw	src;
	unsw	dst;

{	reg	PSTYP	tmp;
	reg	retcd	val;
		retcd	md_okstore(), md_store();

	if (md_okstore(dsp, mode) == ERR)
		return(ERR);

	if ((mode != A_DBGI) && (d_isbpt(src, sz) == ERR)) {
		if (dsp->d_ctxt == C_RUN)
			tmp = md_stop();
		d_mskbpts(dsp);
		val = md_store(dsp, mode, sz, src, dst);
		d_unmskbpts(dsp);
		if (dsp->d_ctxt == C_RUN)
			md_start(tmp);
		return(val);
		}

	return(md_store(dsp, mode, sz, src, dst));
}
