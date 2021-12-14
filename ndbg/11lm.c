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

{	(*dsp->d_puts)("\tUnimplemented ");
	d_lprrdx(((unsl) d_getval(dsp)), dsp->d_ibase);
	(*dsp->d_puts)("  ");
	d_lprrdx(((unsl) d_getval(dsp)), dsp->d_ibase);
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

{	retcd	ddtmov();

	return(ddtmov(sz, src, dst));
}


intern	retcd	md_store(dsp, mode, sz, src, dst)
ddtst	*dsp;
twrd	mode;
unsw	sz;
word	src;
word	dst;

{	retcd	ddtmov();

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

	tmp = ddtgps();
	ddtpps(PSNOINT);
	return(tmp);
}


intern	md_start(ops)
PSTYP	ops;

{	ddtpps(ops);
}
