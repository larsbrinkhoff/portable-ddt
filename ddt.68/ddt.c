/* This file is catted to the begining when all the files are
 * catted together.
 */

#define	DDTNIN		/* make things internal */
#define	DDTCAT		/* so the headers files don't get included again */
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


/* DDT interface that runs with machine stopped. Entry is through
 * machine language startup.
 */


#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"
#include	"optyp.h"


/* DDT control block for hardcore DDT */

ext	char	GETC();
ext	void	PUTC();
ext	void	PUTS();

intern	ddtst d_hcds = { GETC, PUTC, PUTS, C_STOP };
#

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
#

/* This file contains the machine independant routines needed by the
 * hard core debugger. This code is alleged to run properly as a
 * remote debugger front end. You just call the right routine on
 * some external event, and when it returns wait for another external
 * event (possibly at the same time as running a different command
 * processor instance).
 *
 * The way this works in the same machine is that either ddthc or
 * one of the trap routines is called by starting or some external
 * event. These do processing associated with their event, and
 * set up the state of the virtual machine correctly, and then
 * return. The machine language assist will restore the machine
 * state and continue operation.
 *
 * Breakpoints are normally set; they are only cleared when single
 * stepping, either for a real single-step or when single-stepping
 * over a trapped breakpoint. In that case, they are reset when
 * the single-step finishes. While the command processor is running
 * they are set. 
 *
 * Note that the code works fairly hard not to trash memory with
 * breakpoints, etc, left lying around when it is restarted or
 * has an error. It always tries to detect such things and
 * clear them up; things are pretty careful about not setting
 * flags untils the clean-up action is valid.
 *
 * Note that this code (although no other code in the system does
 * so) assumes that ddtps and ddtpc contain the values of the PC
 * and PS that a) were gotten from the debugger when it entered the
 * parser, and b) will be used as the user's on restart.
 *
 * The global flags are pretty obscure but there's no other way to
 * store the state while the code is running. Also, since some pretty
 * strange combinations are allowed, it's not too easy to keep them
 * in a single variable.
 */


#ifndef	DDTCAT

#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"

ext	char	d_crlf[];	/* Need these if not compiled togther */
ext	BPTTYP	d_bpti;
ext	ddtst	d_hcds;

#endif



/* Definitions; currently only says whether code stopped in
 * middle of execute step.
 */


#define	NOXSST	0		/* Not stopped in execute */
#define	XSST	1		/* Did stop there */


/* Static storage for DDT hardcode control. First group is for
 * communication with parser. Last group is communications with
 * machine language support.
 */

intern	unsw	d_go;		/* Start address for $G command */
intern	unsf	d_ncp;		/* Number of times to proceed or step */

intern	unst	d_hlt;		/* Flag for TRC trap caused by HALT */

intern	unst	d_cbpt;		/* Number of current breakpoint plus one */
intern	unst	d_bsst;		/* True if single-stepping over a breakpoint */

intern	unst	d_sst;		/* True if single-stepping */
intern	unst	d_xst;		/* True if single exec'ing */

intern	unst	d_xsst;		/* True if single stepping over transfer */
intern	BPTTYP	*d_xsaddr;	/* Location of replaced instructions */
intern	BPTTYP	*d_xseaddr;	/* End location of replaced instructions */
intern	BPTTYP	d_xsinst[XSNBPT];	/* Instructions after transfer */


ext	PSTYP	ddtps;		/* Register storage while in hardcore */
ext	PCTYP	ddtpc;
ext	word	DEFPC;		/* Default start location */
ext	void	DDT();		/* DDT restart location */




/* Calls the ddt parser and does the right thing when it returns. The
 * right thing is to continue the user program, start at some address,
 * single step, etc. Note way in which it just calls routine to set up
 * execution environment, then returns to machine language support.
 * Note also that it checks to make sure none of the global state
 * flags are on when we get here; they should have been properly cleared
 * in the code. The argument is true if we hit a breakpoint in the middle
 * of an execute step; a simple continue in those cirsumstances resumes
 * the execute step.
 */

intern	d_interp(xsst)
	fwrd	xsst;

{	reg	twrd	cmd;
	twrd	ddtparse();

	for (;;) {
		if ((d_bsst + d_sst + d_xst + d_xsst + d_hlt) != 0)
			ddtbug("pre run st err");

		cmd = ddtparse(&d_hcds);

		switch (cmd) {

		  case C_NONE:	continue;

		  case C_PROC:	if ((d_ncp == 1) && (xsst != NOXSST)) {
					d_xsset(d_xsaddr);
					d_xst++;
					}
				d_proc();
				break;

		  case C_SST:	d_sst++;
				d_sstp();
				break;

		  case C_XST:	d_xst++;
				d_sstp();
				break;

		  case C_STRT:	(*d_hcds.d_puts)(&d_crlf[0]);
				d_exec();
				break;

		  default:	ddtbug("bad run cmd");
		  }

		if ((d_hlt != 0) || ((xsst == NOXSST) &&
		     (((d_bsst + d_sst + d_xst) > 1) ||
		      (((d_bsst + d_sst) > 0) && (d_xsst != 0)))))
			ddtbug("post run st err");

		return;
		}
}


/* Proceed. If currently at a breakpoint, may need to single
 * step through one instruction to insure that the instruction
 * displaced by the current breakpoint will be executed.
 */

intern	d_proc()

{	if (d_cbpt != 0) {
		d_bsst++;
		d_sstp();
		}
}


/* Execute some instructions in single-step mode. If truly 'single-stepping',
 * as opposed to 'execute-stepping', masks all breakpoints; only time
 * breakpoints are masked. They will be reset when the single-step is
 * done. Note that the 'current breakpoint' is always cleared; that way,
 * if we come here as a result of single stepping over a breakpoint, or
 * of doing a proceed after hitting a breakpoint, or whatever, the right
 * thing always happens. Note also that proceeding over a breakpoint
 * in execute-step mode (which may happen if n^X hits a breakpointed
 * intruction inside an executed sequence) can never cause another execute
 * step on the breakpointed instruction.
 */

intern	d_sstp()

{	reg	unst	offset;
		unst	md_isxsi();

	d_cbpt = 0;

	if ((d_xst != 0) && (d_bsst == 0) &&
	    ((offset = md_isxsi(&d_hcds, ddtpc)) != 0)) {
		d_xsset(ddtpc + offset);
		return;
		}		

	d_mskbpts(&d_hcds);
	ddtps |= PSTRC;
}


/* Begin execution of user's program at the specified address.
 * Notice that the system is reset first. Sets redo counter so
 * that when a breakpoint or whatever is hit, it will stop.
 */

intern	d_exec(addr)

{	ddtps = ((PSTYP) DEFPS);
	ddtpc = ((PCTYP) d_go);
	d_cbpt = 0;
	d_ncp = 1;

	ddtexec();
}


/* Code to a single step across a non-local but temporary transfer of
 * execution by setting breakpoints following it. To catch cases where
 * silly assembly language programmers don't return to the place after
 * the instruction, sets up to XSNBPT breakpoints. If you (like
 * UNIX) have data sequences in line with code after such
 * instructions, you.lose.
 */

intern	d_xsset(addr)
reg	BPTTYP	*addr;

{	reg	BPTTYP	*inst;
	reg	unst	i;
	retcd	d_fetch(), d_store();

	if (d_xsst != 0)
		ddtbug("already exec stepping");

	d_xsaddr = addr;
	inst = &d_xsinst[0];

	for (i = 0; i < XSNBPT; i++) {
		if (d_fetch(&d_hcds, A_DBGI, sizeof(BPTTYP), addr, inst)
									== ERR)
			ddtbug("xss no wrt");
		if (d_store(&d_hcds, A_DBGI, sizeof(BPTTYP), &d_bpti, addr)
									== ERR)
			ddtbug("xss no wrt");
		addr++;
		inst++;
		}

	d_xseaddr = addr;
	d_xsst++;
}


/* Routine to replace original instructions. Note that unlike breakpoints,
 * accesses from the run-time debbuger interface do not replace these
 * breakpoints. Since none should be set unless you are ^X'ing through
 * the debugger, you should be OK.
 */

intern	d_xsclr()

{	reg	BPTTYP	*addr;
	reg	BPTTYP	*inst;
	reg	unst	i;

	addr = d_xsaddr;
	inst = &d_xsinst[0];

	for (i = 0; i < XSNBPT; i++) {
		if (d_store(&d_hcds, A_DBGI, sizeof(BPTTYP), inst, addr)
									== ERR)
			ddtbug("xsc no wrt");
		addr++;
		inst++;
		}

	d_xsst = 0;
}



/* Main entry for ddthc; called only on startup. Sets up important
 * registers to something normal looking in case he does an $P.
 * If DDT was restarted, sets things to normal state. Attempts
 * to win when DDT restarted; OK to replace because if flag set
 * relevant data structures have already been set up.
 */

ddthc()

{	ddtinit(&d_hcds);
	(*d_hcds.d_puts)(MCHSTR);
	(*d_hcds.d_puts)(" DDT execution; Manual restart = ");
	d_lprrdx(&d_hcds, ((unsl) DDT), d_hcds.d_ibase);
	(*d_hcds.d_puts)("\r\n\n");

	if (d_xsst != 0)
		d_xsclr();
	if ((d_sst != 0) || (d_xst != 0) || (d_bsst != 0))
		d_unmskbpts(&d_hcds);

	d_initbpts();
	d_clrst();

	ddtps = ((PSTYP) DEFPS);
	ddtpc = ((PCTYP) &DEFPC);

	d_interp(NOXSST);
}


/* Clears hard core state. Called on startup, and also on errors.
 */

intern	d_clrst()

{	d_cbpt = 0;
	d_bsst = 0;
	d_sst = 0;
	d_xst = 0;
	d_xsst = 0;
	d_hlt = 0;
}


/* Called on occurrence of a breakpoint trap. First, backs up PC to
 * be on the breakpoint (if it was a real breakpoint, it will be
 * adjusted back after checking). First, check to see if we were
 * execute stepping over an inst. If so, replace instructions
 * from location after inst and continue single stepping. Otherwise,
 * look to see which breakpoint we hit, and if still some continues
 * left, do them. Otherwise, figures out which breakpoint occurred,
 * and displays breakpoint message. Then jumps off to command
 * interpreter.
 *
 * It's not completely clear what the right thing to do is if a
 * breakpoint is hit during an execute step. The current action is
 * that if the number of continues is non-zero it continues, decrementing
 * by one, otherwise it hits the breakpoint. You can $P out of such
 * a breakpoint, and it will resume the ^X, but anything other than
 * a simple $P causes the ^X to be lost. Other possibilities are
 * not to have breakpoints set at all during execute step at all,
 * having continues not count.
 */

ddtbpt()

{	reg	BPTTYP	*addr;
		unst	d_findbpt();
		unst	md_prinst();

	ddtpc -= BPTOFF;
	addr = ((BPTTYP *) ddtpc);

	if ((d_xsst != 0) && ((addr >= d_xsaddr) && (addr < d_xseaddr))) {
		d_xsclr();
		ddttrc();
		return;
		}

	d_cbpt = d_findbpt(&d_hcds, ((unsw) ddtpc));
	if (d_cbpt == 0) {
		ddtpc += BPTOFF;
		ddtrte("BPT");
		return;
		}

	if (--d_ncp > 0) {
		d_proc();
		return;
		}

	(*d_hcds.d_puts)("\r\n$");
	d_lprrdx(&d_hcds, ((unsl) (d_cbpt - 1)), 10);
	(*d_hcds.d_puts)("B >> ");
	d_prloc(&d_hcds, ddtpc);
	(*d_hcds.d_puts)("\t{ ");
	md_prinst(&d_hcds, ddtpc);
	(*d_hcds.d_puts)(" }\t");

	if (d_xsst == 0) {
		d_interp(NOXSST);
		return;
		}

	d_xsclr();
	d_xst = 0;
	d_interp(XSST);
}


/* The trace trap handler. Trace traps occur during single-stepping,
 * and in the process of proceeding from a breakpoint. If we are
 * proceeding from a breakpoint, continue. Note that trace traps do
 * not count toward the repetition count on breakpoints. If we are
 * single-stepping, and the count ran out, just call the command
 * interpreter. Note that we must always reinsert the breakpoint
 * instructions which where temporarily removed when we started
 * single-stepping after single-stepping is done.
 */

ddttrc()

{	unst	md_prinst();

	ddtps &= ~PSTRC;
	if (d_bsst != 0) {
		d_bsst = 0;
		d_unmskbpts(&d_hcds);
		return;
		}

	if ((d_sst != 0) || (d_xst != 0) || (d_hlt != 0)) {

		if (d_hlt == 0) {
			if (--d_ncp > 0) {
				d_sstp();
				return;
				}
			if (d_sst != 0)
				d_sst = 0;
			  else
				d_xst = 0;
			d_unmskbpts(&d_hcds);
			}
		  else
			d_hlt = 0;

		d_hcds.d_dot = ((unsw) ddtpc);
		(*d_hcds.d_puts)("\r\n>> ");
		d_prloc(&d_hcds, ((unsw) ddtpc));
		(*d_hcds.d_puts)("\t{ ");
		md_prinst(&d_hcds, ((unsw) ddtpc));
		(*d_hcds.d_puts)(" }\t");

		d_interp(NOXSST);
		return;
		}

	ddtrte("TRC");
}


/* Called on runtime error; clears state so that if we were doing single
 * step, whatever, won't show. Tries to restore memory to consistent
 * state.
 */

ddtrte(errstr)
char	*errstr;

{	if (d_xsst != 0)
		d_xsclr();

	if ((d_sst != 0) || (d_xst != 0) || (d_bsst != 0) || (d_hlt != 0)) {
		ddtps &= ~PSTRC;
		if (d_hlt == 0)
			d_unmskbpts(&d_hcds);
		}
		
	d_clrst();
	(*d_hcds.d_puts)(&d_crlf[0]);
	d_prloc(&d_hcds, ((unsw) ddtpc));
	(*d_hcds.d_puts)("; ");
	(*d_hcds.d_puts)(errstr);
	(*d_hcds.d_puts)("\t");
	d_interp(NOXSST);
}
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

	  case ']':
	  case '[':
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
			
	if ((dcp->dc_cmd == 0) || ((dsp->d_inpst == S_TESC) &&
	     ((dcp->dc_typ == 0) || (d_numval(dsp) != 0))) ||
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
#
/****************************************************************************

	DEBUGGER - 68000 instruction printout

****************************************************************************/

#ifndef DDTCAT	/* don't reget headers if catting when all routines internal */
#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"
#endif

static short	radix;
static unsl	dot;
static ddtst	*ldsp;

static int	dotinc;

static char *badop = "\t???";
static char *IMDF;				/* immediate data format */

static char *bname[16] = { "ra", "sr", "hi", "ls", "cc", "cs", "ne",
		    "eq", "vc", "vs", "pl", "mi", "ge", "lt", "gt", "le" };

static char *shro[4] = { "as", "ls", "rox", "ro" };

static char *bit[4] = { "btst", "bchg", "bclr", "bset" };

static int omove(),obranch(),oimmed(),oprint(),oneop(),soneop(),oreg(),ochk();
static int olink(),omovem(),oquick(),omoveq(),otrap(),oscc(),opmode(),shroi();
static int extend(),biti();

static struct opdesc
{			
	unsigned short mask, match;
	int (*opfun)();
	char *farg;
} opdecode[] =
{					/* order is important below */
  0xF000, 0x1000, omove, "b",		/* move instructions */
  0xF000, 0x2000, omove, "l",
  0xF000, 0x3000, omove, "w",
  0xF000, 0x6000, obranch, 0,		/* branches */
  0xFF00, 0x0000, oimmed, "or",		/* op class 0  */
  0xFF00, 0x0200, oimmed, "and",
  0xFF00, 0x0400, oimmed, "sub",
  0xFF00, 0x0600, oimmed, "add",
  0xFF00, 0x0A00, oimmed, "eor",
  0xFF00, 0x0C00, oimmed, "cmp",
  0xF100, 0x0100, biti, 0,
  0xF800, 0x0800, biti, 0,
  0xFFC0, 0x40C0, oneop, "move_from_sr", /* op class 4 */
  0xFF00, 0x4000, soneop, "negx",
  0xFF00, 0x4200, soneop, "clr",
  0xFFC0, 0x44C0, oneop, "move_to_ccr",
  0xFF00, 0x4400, soneop, "neg",
  0xFFC0, 0x46C0, oneop, "move_to_sr",
  0xFF00, 0x4600, soneop, "not",
  0xFFC0, 0x4800, oneop, "nbcd",
  0xFFF8, 0x4840, oreg, "swap\td%D",
  0xFFC0, 0x4840, oneop, "pea",
  0xFFF8, 0x4880, oreg, "extw\td%D",
  0xFFF8, 0x48C0, oreg, "extl\td%D",
  0xFB80, 0x4880, omovem, 0,
  0xFFC0, 0x4AC0, oneop, "tas",
  0xFF00, 0x4A00, soneop, "tst",
  0xFFF0, 0x4E40, otrap, 0,
  0xFFF8, 0x4E50, olink, 0,
  0xFFF8, 0x4880, oreg, "extw\td%D",
  0xFFF8, 0x48C0, oreg, "extl\td%D",
  0xFFF8, 0x4E58, oreg, "unlk\ta%D",
  0xFFF8, 0x4E60, oreg, "move\ta%D,usp",
  0xFFF8, 0x4E68, oreg, "move\tusp,a%D",
  0xFFFF, 0x4E70, oprint, "reset",
  0xFFFF, 0x4E71, oprint, "nop",
  0xFFFF, 0x4E72, oprint, "stop",
  0xFFFF, 0x4E73, oprint, "rte",
  0xFFFF, 0x4E75, oprint, "rts",
  0xFFFF, 0x4E76, oprint, "trapv",
  0xFFFF, 0x4E77, oprint, "rtr",
  0xFFC0, 0x4E80, oneop, "jsr",
  0xFFC0, 0x4EC0, oneop, "jmp",
  0xF1C0, 0x4180, ochk, "chk",
  0xF1C0, 0x41C0, ochk, "lea",
  0xF0C0, 0x50C0, oscc, 0,
  0xF100, 0x5000, oquick, "addq",
  0xF100, 0x5100, oquick, "subq",
  0xF000, 0x7000, omoveq, 0,
  0xF1C0, 0x80C0, ochk, "divu",
  0xF1C0, 0x81C0, ochk, "divs",
  0xF1F0, 0x8100, extend, "sbcd",
  0xF000, 0x8000, opmode, "or",
  0xF1C0, 0x91C0, opmode, "sub",
  0xF130, 0x9100, extend, "subx",
  0xF000, 0x9000, opmode, "sub",
  0xF1C0, 0xB1C0, opmode, "cmp",
  0xF138, 0xB108, extend, "cmpm",
  0xF100, 0xB000, opmode, "cmp",
  0xF100, 0xB100, opmode, "eor",
  0xF1C0, 0xC0C0, ochk, "mulu",
  0xF1C0, 0xC1C0, ochk, "muls",
  0xF1F8, 0xC188, extend, "exg",
  0xF1F8, 0xC148, extend, "exg",
  0xF1F8, 0xC140, extend, "exg",
  0xF1F0, 0xC100, extend, "abcd",
  0xF000, 0xC000, opmode, "and",
  0xF1C0, 0xD1C0, opmode, "add",
  0xF130, 0xD100, extend, "addx",
  0xF000, 0xD000, opmode, "add",
  0xF100, 0xE000, shroi, "r",
  0xF100, 0xE100, shroi, "l",
  0, 0, 0, 0
};

unsb	md_prinst(dsp, addr)
reg	ddtst	*dsp;
	unsw	addr;
{
	unss	inst;
	register struct opdesc *p;
	register int (*fun)();

	d_mskbpts(dsp);		/* this will lose big in run mode */
	ldsp = dsp;
	radix = dsp->d_obase;
	dot = addr;
	inst = *(unss *)dot;
/*	dot += 2; */
	dotinc = 2;
	switch (radix) {
		case 8:
			IMDF = "#%O";
			break;
		case 10:
			IMDF = "#%D";
			break;
		default:
			IMDF = "#%X";
			break;
		}
	for (p = opdecode; p->mask; p++)
		if ((inst & p->mask) == p->match) break;
	if (p->mask != 0) {
		(*p->opfun)(inst, p->farg);
		d_unmskbpts(dsp);
		return dotinc;
	}
	else {
		printf(badop);
		d_unmskbpts(dsp);
		return 2;
	}
}

printea(mode,regstr,size)
long mode, regstr;
int size;
{
	long index,disp;

	switch ((int)(mode)) {
	  case 0:	printf("d%D",regstr);
			break;

	  case 1:	printf("a%D",regstr);
			break;

	  case 2:	printf("a%D@",regstr);
			break;

	  case 3:	printf("a%D@+",regstr);
			break;

	  case 4:	printf("a%D@-",regstr);
			break;

	  case 5:	printf("a%D@(%D.)",regstr,instfetch(2));
			break;

	  case 6:	index = instfetch(2);
			disp = (char)(index&0377);
			printf("a%d@(%d,%c%D%c)",regstr,disp,
			  (index&0100000)?'a':'d',(index>>12)&07,
			  (index&04000)?'l':'w');
			break;

	  case 7:	switch ((int)(regstr))
			{
			  case 0:	index = instfetch(2);
					printf("%x:w",index);
					break;

			  case 1:	index = instfetch(4);
			  		d_prloc(ldsp, index);
					break;

			  case 2:	printf("pc@(%D.)",instfetch(2));
					break;

			  case 3:	index = instfetch(2);
					disp = (char)(index&0377);
					printf("a%D@(%D,%c%D:%c)",regstr,disp,
        			        (index&0100000)?'a':'d',(index>>12)&07,
					(index&04000)?'l':'w');
					break;

			  case 4:	index = instfetch(size==4?4:2);
					printf(IMDF, index);
					break;

			  default:	printf("???");
					break;
			}
			break;

	  default:	printf("???");
	}
}

printEA(ea,size)
long ea;
int size;
{
	printea((ea>>3)&07,ea&07,size);
}

mapsize(inst)
long inst;
{
	inst >>= 6;
	inst &= 03;
	return((inst==0) ? 1 : (inst==1) ? 2 : (inst==2) ? 4 : -1);
}

char suffix(size)
register int size;
{
	return((size==1) ? 'b' : (size==2) ? 'w' : (size==4) ? 'l' : '?');
}

static omove(inst, s)
long inst;
char *s;
{
	int size;

	printf("mov%c\t",*s);
	size = ((*s == 'b') ? 1 : (*s == 'w') ? 2 : 4);
	printea((inst>>3)&07,inst&07,size);
	printf(",");
	printea((inst>>6)&07,(inst>>9)&07,size);
}

static obranch(inst,dummy)
long inst;
{
	long disp = inst & 0377;
	char *s; 

	s = "s ";
	if (disp > 127) disp |= ~0377;
	else if (disp == 0) { s = " "; disp = instfetch(2); }
	printf("b%s%s\t",bname[(int)((inst>>8)&017)],s);
	d_prloc(ldsp, disp + inkdot(2));
}

static oscc(inst,dummy)
long inst;
{
	printf("s%s\t",bname[(int)((inst>>8)&017)]);
	printea((inst>>3)&07,inst&07,1);
}

static biti(inst, dummy)
long inst;
{
	printf("%s\t", bit[(int)((inst>>6)&03)]);
	if (inst&0x0100) printf("d%D,", inst>>9);
	else { printf(IMDF, instfetch(2)); printf(","); }
	printEA(inst);
}

static opmode(inst,opcode)
long inst;
{
	register int opmode = (int)((inst>>6) & 07);
	register int regstr = (int)((inst>>9) & 07);
	int size;

	size = (opmode==0 || opmode==4) ?
		1 : (opmode==1 || opmode==3 || opmode==5) ? 2 : 4;
	printf("%s%c\t", opcode, suffix(size));
	if (opmode>=4 && opmode<=6)
	{
		printf("d%d,",regstr);
		printea((inst>>3)&07,inst&07, size);
	}
	else
	{
		printea((inst>>3)&07,inst&07, size);
		printf(",%c%d",(opmode<=2)?'d':'a',regstr);
	}
}

static shroi(inst,ds)
long inst;
char *ds;
{
	int rx, ry;
	char *opcode;
	if ((inst & 0xC0) == 0xC0)
	{
		opcode = shro[(int)((inst>>9)&03)];
		printf("%s%s\t", opcode, ds);
		printEA(inst);
	}
	else
	{
		opcode = shro[(int)((inst>>3)&03)];
		printf("%s%s%c\t", opcode, ds, suffix(mapsize(inst)));
		rx = (int)((inst>>9)&07); ry = (int)(inst&07);
		if ((inst>>5)&01) printf("d%d,d%d", rx, ry);
		else
		{
			printf(IMDF, (rx ? rx : 8));
			printf(",d%d", ry);
		}
	}
}		

static oimmed(inst,opcode) 
long inst;
register char *opcode;
{
	register int size = mapsize(inst);
	long const;

	if (size > 0)
	{
		const = instfetch(size==4?4:2);
		printf("%s%c\t", opcode, suffix(size));
		printf(IMDF, const); printf(",");
		printEA(inst,size);
	}
	else printf(badop);
}

static oreg(inst,opcode)
long inst;
register char *opcode;
{
	printf(opcode, (inst & 07));
}

static extend(inst, opcode)
long	inst;
char	*opcode;
{
	register int size = mapsize(inst);
	int ry = (inst&07), rx = ((inst>>9)&07);
	char c;

	c = ((inst & 0x1000) ? suffix(size) : ' ');
	printf("%s%c\t", opcode, c);
	if (*opcode == 'e')
	{
		if (inst & 0x0080) printf("d%D,a%D", rx, ry);
		else if (inst & 0x0008) printf("a%D,a%D", rx, ry);
		else printf("d%D,d%D", rx, ry);
	}
	else if ((inst & 0xF000) == 0xB000) printf("a%D@+,a%D@+", ry, rx);
	else if (inst & 0x8) printf("a%D@-,a%D@-", ry, rx);
	else printf("d%D,d%D", ry, rx);
}

static olink(inst,dummy)
long inst;
{
	printf("link\ta%D,", inst&07);
	printf(IMDF, instfetch(2));
}

static otrap(inst,dummy)
long inst;
{
	printf("trap\t");
	printf(IMDF, inst&017);
}

static oneop(inst,opcode)
long inst;
register char *opcode;
{
	printf("%s\t",opcode);
	printEA(inst);
}

pregmask(mask)
register int mask;
{
	register int i;
	register int flag = 0;

	printf("#<");
	for (i=0; i<16; i++)
	{
		if (mask&1)
		{
			if (flag) printf(","); else flag++;
			printf("%c%d",(i<8)?'d':'a',i&07);
		}
		mask >>= 1;
	}
	printf(">");
}

static omovem(inst,dummy)
long inst;
{
	register int i, list = 0, mask = 0100000;
	register int reglist = (int)(instfetch(2));

	if ((inst & 070) == 040)	/* predecrement */
	{
		for(i = 15; i > 0; i -= 2)
		{ list |= ((mask & reglist) >> i); mask >>= 1; }
		for(i = 1; i < 16; i += 2)
		{ list |= ((mask & reglist) << i); mask >>= 1; }
		reglist = list;
	}
	printf("movem%c\t",(inst&100)?'l':'w');
	if (inst&02000)
	{
		printEA(inst);
		printf(",");
		pregmask(reglist);
	}
	else
	{
		pregmask(reglist);
		printf(",");
		printEA(inst);
	}
}

static ochk(inst,opcode)
long inst;
register char *opcode;
{
	printf("%s\t",opcode);
	printEA(inst);
	printf(",%c%D",(*opcode=='l')?'a':'d',(inst>>9)&07);
}

static soneop(inst,opcode)
long inst;
register char *opcode;
{
	register int size = mapsize(inst);

	if (size > 0)
	{
		printf("%s%c\t",opcode,suffix(size));
		printEA(inst);
	}
	else printf(badop);
}

static oquick(inst,opcode)
long inst;
register char *opcode;
{
	register int size = mapsize(inst);
	register int data = (int)((inst>>9) & 07);

	if (data == 0) data = 8;
	if (size > 0)
	{
		printf("%s%c\t", opcode, suffix(size));
		printf(IMDF, data); printf(",");
		printEA(inst);
	}
	else printf(badop);
}

static omoveq(inst,dummy)
long inst;
{
	register int data = (int)(inst & 0377);

	if (data > 127) data |= ~0377;
	printf("moveq\t"); printf(IMDF, data);
	printf(",d%D", (inst>>9)&07);
}

static oprint(inst,opcode)
long inst;
register char *opcode;
{
	printf("%s",opcode);
}


long
inkdot(n)
int n;
{
	return(dot+n);
}


long
instfetch(size)
int size;
{
	register long l1,l2;

	if (size == 4) {
		l1 = *(long *)(inkdot(dotinc));
		dotinc += 2;
	} else {
		l1 = (long)(*(short *)(inkdot(dotinc)));
		if (l1 & 0x8000) l1 |= 0xFFFF0000;
	}
	dotinc += 2;
	return(l1);
}

#define	JMASK	0xffc0
#define	JVAL	0x4e80
#define	BMASK	0xff00
#define	BVAL	0x6100

/* Returns 0 if instruction at addr can be ^X'd. */
intern unst	md_isxsi(dsp, addr)
reg	ddtst	*dsp;
unss	*addr;
{
	reg	swrd	inst;
		retcd	d_fetch();

	if (d_fetch(dsp, A_DBGI, sizeof(swrd), addr, &dsp->d_swrd) == ERR)
		ddtbug("no pc fetch");
	inst = dsp->d_swrd;

	if ((inst & BMASK) == BVAL) {
		if ((inst & 0xff) == 0) return 4;
		else return 2;
	}
	
	if ((inst & JMASK) == JVAL) {
		switch (inst & 070) {
		case 020: return 2;
		case 050:
		case 060: return 4;
		case 070:
			switch (inst & 07) {
			case 0: return 4;
			case 1: return 6;
			case 2: return 4;
			case 3: return 4;
			default: return 0;
			}
		default: return 0;
		}
	}
	
	return 0;
}
#

/* This file includes all the routines to handle all operations
 * that will differ from machine to machine and which will
 * differ depending on the operating mode of the machine. Thus,
 * if the parser, etc are operating as a front end on a distant
 * network machine, these routines would involve network interface
 * work.
 */


#ifndef	DDTCAT	/* Don't reget hdrs if catting when all routines internal */
#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"
#endif



/* Machine dependant mode commands */

intern	retcd	md_mdepmd(dsp)
reg	ddtst	*dsp;
{	if (d_numval(dsp) != 2)
		return(ERR);

	printf("\tUnimplemented %o %o", d_getval(dsp), d_getval(dsp));
}


/* Checks to see if current memory op is legal in this mode.
 */

intern	retcd	md_okfetch(dsp, mode)

{	return(OK);
}


intern	retcd	md_okstore(dsp, mode)

{	return(OK);
}


/* Does fetch or store, doing machine dependant mapping if necessary.
 * For remote debugger, would access memory over network.
 */

intern	retcd	md_fetch(dsp, mode, sz, src, dst)
ddtst	*dsp;
twrd	mode;
unsw	sz;
word	src;
word	dst;

{	unsb	ddtmov();

	return(ddtmov(sz, src, dst));
}


intern	retcd	md_store(dsp, mode, sz, src, dst)
ddtst	*dsp;
twrd	mode;
unsw	sz;
word	src;
word	dst;

{	unsb	ddtmov();

	return(ddtmov(sz, src, dst));
}


/* Stop or start processor from running normal code. On machine where
 * debugger running locally, just ups PS. For remote machine, would
 * cause it to drop into command listen mode. Note returns PS machine
 * stopped with; reset when restarted. This is used by the ^B command
 * of the run-time debugger, which causes a trace trap as the simplest
 * way of getting into the hard-core debugger.
 */

intern	PSTYP	md_stop()

{	reg	PSTYP	tmp;
/*
	tmp = ddtgps();
	ddtpps(PSNOINT);
	return(tmp);	only needed for run time ddt */
}


intern	md_start(ops)
PSTYP	ops;

{ /*	ddtpps(ops);	only needed for run time ddt */
}


unsb	ddtmov(sz, src, dst)
/*reg*/	unsw	sz;		/* the compiler losses it it you try to
				 * return a register variable */
reg	word	src;
reg	word	dst;

{	if (sz == sizeof(byte)) {
		*((byte *) dst) = *((byte *) src);
		return(sz);
		};

	if (((src & 1) != 0) || ((dst & 1) != 0))
		return(0);

	if (sz == sizeof(swrd)) {
		*((swrd *) dst) = *((swrd *) src);
		return(sz);
		};

	if (sz == sizeof(lwrd)) {
		*((lwrd *) dst) = *((lwrd *) src);
		return(sz);
		};

	ddtbug("bad sz in mov");
	return(0);
}

ddtbug(msg)
char	*msg;
{
	puts("FATAL ERR! ");
	puts(msg);
	for (;;) ;
}

ddterr()
{
	puts("Internal DDT error, restarting\n");
	DDT();
}
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

