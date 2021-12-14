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


/* This file includes all the routines to handle breakpoints.
 * The way these work is that breakpoints are active all the
 * time normally, to simplify the case of doing breakpoints
 * from RUN mode, as well as to make it easy to check whether a
 * breakpoint is legal. They are only unmasked when a fetch or store
 * would touch a breakpointed location, and when single-stepping.
 */


#ifndef	DDTCAT	/* Don't reget hdrs if catting when all routines internal */
#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"
#endif



/* Some command codes */

#define	B_ALL	-1			/* Remove all breakpoints */
#define	B_ANY	-1			/* Use any breakpoint */


/* Breakpoint information table */

#define	bpt	struct	bptstr

bpt	{	unsw	b_addr;		/* Breakpoint address */
		BPTTYP	b_inst;		/* Displaced instruction */
		unst	b_actv;		/* Breakpoint active flag */
		unst	b_bptno;	/* Breakpoint number */
	};


/* Breakpoint information tables */

intern	bpt	d_bpts[BPTSIZ];		/* Better be initialized to 0! */
#define	EBPT	(&d_bpts[BPTSIZ])	/* Address off end of buffer */

intern	BPTTYP	d_bpti = BPTINST;	/* Actual breakpoint instruction */


/* Useful string */

intern	char	d_crlf[] =	"\n\r";



/* Initialize breakpoint system; just sets up numbers.
 */

intern	d_initbpts()

{	reg	bpt	*bp;
	reg	unst	i;

	for (i = 0, bp = &d_bpts[0]; bp < EBPT; i++, bp++)
		bp->b_bptno = i;
}


/* List all active breakpoints. Note that breakpoints are numbered
 * in decimal (since their numbers are read in decimal useually).
 */

intern	d_lstbpts(dsp)
reg	ddtst	*dsp;

{	reg	bpt	*bp;

	for (bp = &d_bpts[0]; bp < EBPT; bp++) {
		if (bp->b_actv == 0)
			continue;

		(*dsp->d_puts)(&d_crlf[0]);
		d_lprrdx(dsp, ((unsl) bp->b_bptno), 10);
		(*dsp->d_puts)(": ");
		d_prloc(dsp, bp->b_addr);
		}
}


/* Figure out what BPT related command we are doing (can tell by
 * number and location of args) and go do it. Notice that tmp
 * is used to prevent C argument allocation order side effect
 * randomness. Returns ERR if command was bad.
 */

intern	retcd	d_dobptcmd(dsp)
reg	ddtst	*dsp;

{	reg	unsw	tmp;
		retcd	d_rmbpt();
		retcd	d_setbpt();
		unsw	d_getval();

	if (d_numval(dsp) == 0)
		return(d_rmbpt(dsp, B_ALL));

	if (d_numval(dsp) == 2) {
		if (d_numeval(dsp) != 1)
			return(ERR);
		tmp = d_getval(dsp);
		return(d_setbpt(dsp, d_getval(dsp), tmp));
		}

	if (d_numeval(dsp) == 0)
		return(d_rmbpt(dsp, d_getval(dsp)));

	return(d_setbpt(dsp, B_ANY, d_getval(dsp)));
}


/* Set breakpoint number bptno (or the first available breakpoint,
 * B_ANY) at the specified address. The breakpoint is set right away.
 *
 * Trying to set a breakpoint with the machine stopped in the
 * middle of trying to set one while running will cause major fuckups.
 * (This is for cases where the debugger is running locally.)
 * Not too easy to fix without locking out interrupts.
 */

intern	retcd	d_setbpt(dsp, bptno, addr)
reg	ddtst	*dsp;
reg	intf	bptno;
xreg	unsw	addr;

{	reg	bpt	*bp;
	retcd	d_fetch(), d_store();

	for (bp = &d_bpts[0]; bp < EBPT; bp++) {
		if (bp->b_actv == 0) {
			if (bptno == B_ANY)
				bptno = bp->b_bptno;
			continue;
			}
		if (bp->b_addr == addr)
			return(ERR);
		}

	if ((bptno == B_ANY) || (bptno >= BPTSIZ))
		return(ERR);

	bp = &d_bpts[bptno];

	if (bp->b_actv != 0) {
		if (d_store(dsp, A_DBGI, sizeof(BPTTYP), &bp->b_inst,
							bp->b_addr) == ERR)
			ddtbug("bpt no wrt reset");
		bp->b_actv = 0;
		}

	bp->b_addr = addr;
	if (d_fetch(dsp, A_DBGI, sizeof(BPTTYP), addr, &bp->b_inst) == ERR)
		return(ERR);

	bp->b_actv++;

	if (d_store(dsp, A_DBGI, sizeof(BPTTYP), &d_bpti, addr) == ERR) {
		bp->b_actv = 0;
		return(ERR);
		}

	return(OK);
}


/* Remove breakpoint number bptno (or all breakpoints if flag is B_ALL).
 * Barfs on error since should be impossible.
 *
 * Same comments about interrupts apply.
 */

intern	retcd	d_rmbpt(dsp, bptno)
reg	ddtst	*dsp;
reg	intf	bptno;

{	reg	bpt	*bp;

	if (bptno >=  BPTSIZ)
		return(ERR);

	if (bptno != B_ALL) {
		bp = &d_bpts[bptno];
		if (bp->b_actv == 0)
			return(ERR);

		if (d_store(dsp, A_DBGI, sizeof(BPTTYP), &bp->b_inst,
							bp->b_addr) == ERR)
			ddtbug("bpt no wrt rem");

		bp->b_actv = 0;
		return(OK);
		}

	for (bp = &d_bpts[0]; bp < EBPT; bp++) {
		if (bp->b_actv == 0)
			continue;
		if (d_store(dsp, A_DBGI, sizeof(BPTTYP), &bp->b_inst,
							 bp->b_addr) == ERR)
			ddtbug("bpt no wrt rem");

		bp->b_actv = 0;
		}

	return(OK);
}


/* Returns ERR if the specified location lies within a breakpoint
 * instruction. Used in fetch and store to see if they need to
 * mask breakpoints off before going ahead.
 */

intern	retcd	d_isbpt(loc, len)
	unsw	loc;
	unst	len;

{	reg	bpt	*bp;
	reg	unsw	bptloc;
		unsw	ebptloc;
	reg	unsw	tloc;
		unsw	eloc;

	eloc = (loc + len);

	for (bp = &d_bpts[0]; bp < EBPT; bp++) {
		if (bp->b_actv == 0)
			continue;

		bptloc = bp->b_addr;
		ebptloc = (bptloc + sizeof(BPTTYP));
		if ((((tloc = loc) >= bptloc) && (tloc < ebptloc)) ||
		    (((tloc = eloc) > bptloc) && (tloc <= ebptloc)))
			return(ERR);
		}

	return(OK);
}


/* Mask off all currently active breakpoints so the user won't see them
 * when he examines memory, but leave them active so when he returns
 * to the program they can be unmasked.
 *
 * This and the next routine should be run with the machine stopped.
 * When the debugger is in the same machine this means turning off
 * interrupts.
 *
 * Comments above about errors hold.
 */

intern	d_mskbpts(dsp)
reg	ddtst	*dsp;

{	reg	bpt	*bp;
	
	for (bp = &d_bpts[0]; bp < EBPT; bp++) {
		if (bp->b_actv == 0)
			continue;
		if (d_store(dsp, A_DBGI, sizeof(BPTTYP), &bp->b_inst,
							bp->b_addr) == ERR)
			ddtbug("bpt no wrt msk");
		}
}


/* Unmask all currently active breakpoints. Note that since the
 * instruction was saved when the BPY was set, no need to do so here.
 */

intern	d_unmskbpts(dsp)
reg	ddtst	*dsp;

{	reg	bpt	*bp;

	for (bp = &d_bpts[0]; bp < EBPT; bp++) {
		if (bp->b_actv == 0)
			continue;
		if (d_store(dsp, A_DBGI, sizeof(BPTTYP), &d_bpti,
							bp->b_addr) == ERR)
			ddtbug("bpt no wrt unmsk");
		}
}


/* Find breakpoint; looks through breakpoint table to see
 * if there is a breakpoint at this address; if so, returns
 * number (plus one, so that zero exists as a distunguished
 * value for none).
 */

intern	unst	d_findbpt(dsp, addr)
reg	ddtst	*dsp;
reg	unsw	addr;

{	reg	bpt	*bp;

	for (bp = &d_bpts[0]; bp < EBPT; bp++) {
		if (bp->b_addr != addr)
			continue;

		return(bp->b_bptno + 1);
		}

	return(0);
}
