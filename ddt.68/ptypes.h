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


/* Arrange for correct types file to be included.
 */

#ifdef	pdp11
#include	"11types.h"
#endif

#ifdef	MC68000
#include	"68types.h"
#endif

#ifdef	vax
#include	"vax-types.h"
#endif

#ifndef	byte
struct x { ERROR ERROR; };	/* Not for a supported CPU type */
#endif
