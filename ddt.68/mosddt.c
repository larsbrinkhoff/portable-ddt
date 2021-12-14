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


/* DDT interface that runs under MOS; good for examining memory, etc.
 * Does everything except execution commands.
 */


#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"



/* DDT control block for MOSDDT */

ext	char	bin();
ext	void	bout();
ext	void	csout();

intern	ddtst	d_mosds =	{ &bin, &bout, &csout, C_RUN };



/* Top level; initialization and call parser. Should never
 * return.
 */

MOSDBG()

{	ddtinit(&d_mosds);
	(*d_mosds.d_puts)("MOS DDT\r\n\n");
	ddtparse(&d_mosds);
	ddtbug("MOSDDT return");
}
