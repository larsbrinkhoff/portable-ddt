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
