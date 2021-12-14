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
