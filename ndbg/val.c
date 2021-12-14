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


/* This file contains the subroutines for hacking the value storage.
 * There are some simple macros as well (in the header file) which tell
 * how many values have been typed and whether a value is open,
 * for use in other packages.
 */


#ifndef	DDTCAT	/* Don't reget hdrs if catting when all routines internal */
#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"
#endif

#ifdef	DDTDBG
intern	char	d_valerr[] =	"bd val st";
#endif



/* Sets the value of the saved item, using the old state of the input
 * machine and the value just read, as well as the old value, if using an
 * operator. Note that using an operator without having an open value
 * is an error. d_svalf always points to either the next available
 * value repository, if no value is open, or the current one if a
 * value is open. d_svalo is a flag which indicates that a value
 * is open.
 */

intern	d_setval(dsp, tmp)
reg	ddtst	*dsp;
	unsw	tmp;

{	switch(dsp->d_inpst) {

	  case S_NONE:
	  case S_ESC:
	  case S_TESC:	if (dsp->d_svalf >= VALSIZ) {
				d_err(dsp);
				return;
				}
			if (dsp->d_svalo++ != 0)
				ddtbug(d_valerr);
			dsp->d_svalv[dsp->d_svalf] = tmp;
			return;

	  case S_ADD:	dsp->d_svalv[dsp->d_svalf] += tmp;
			dsp->d_inpst = S_NONE;
			break;

	  case S_SUB:	dsp->d_svalv[dsp->d_svalf] -= tmp;
			dsp->d_inpst = S_NONE;
			break;

	  case S_MUL:	dsp->d_svalv[dsp->d_svalf] *= tmp;
			dsp->d_inpst = S_NONE;
			break;

	  default:	ddtbug(d_valerr);
	  }	

	if (dsp->d_svalo == 0)
		ddtbug(d_valerr);
}


/* Marks a value as having been completely entered. d_svalf is incremented
 * (which also shows that the value exists) when an operator which definitely
 * indicates the end of a value is seen.
 */

intern	d_endval(dsp)
reg	ddtst	*dsp;

{	if ((dsp->d_svalo == 0) ||
	    ((dsp->d_inpst != S_NONE) &&
	     (dsp->d_inpst != S_ESC) &&
	     (dsp->d_inpst != S_TESC)))
		ddtbug(d_valerr);
	dsp->d_svalo = 0;
	dsp->d_svalf++;
}


/* Notes the number of values typed before a '$' is seen.
 */

intern	d_seteval(dsp)
reg	ddtst	*dsp;

{	if (dsp->d_svalo != 0)
		ddtbug(d_valerr);
	dsp->d_svale = dsp->d_svalf;
}


/* Return the first typed value; shift the rest over.
 */

intern	unsw	d_getval(dsp)
reg	ddtst	*dsp;

{	reg	unsw	tmp;
	reg	unst	i;

	if ((dsp->d_svalo != 0) || (dsp->d_svalf == 0))
		ddtbug(d_valerr);

	tmp = dsp->d_svalv[0];
	dsp->d_svalf--;
	for (i = 0; i < dsp->d_svalf; i++)
		dsp->d_svalv[i] = dsp->d_svalv[(i + 1)];
	return(tmp);
}
