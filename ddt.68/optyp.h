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


/* Arrange for correct machine and operating environment
 * dependant file to be included.
 */

#ifdef	DDTLM
#ifdef	PDP11
#include	"11lm.h"
#endif
#endif

#ifdef	DDTLM
#ifdef	MC68000
#include	"68lm.h"
#endif
#endif

#ifdef	DDTUNIX
#ifdef	PDP11
#include	"11unix.h"
#endif
#endif

#ifndef	PUTC
struct x { ERROR ERROR; };	/* Not for a supported CPU type */
#endif

