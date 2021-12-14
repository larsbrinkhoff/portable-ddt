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
