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


/* This files contains definitons intended to help make code
 * portable. C programs, although usually easily portable,
 * often are not exactly portable.
 */

/* This section includes data type definitions, done in
 * a regular, portable, fashion. It provides rough
 * equivalents in cases where the exact types don't exist.
 * Basically, data items are either a) specific length, with
 * or without type information; b) the word size of the machine,
 * where a word is defined to be a thing large enough to hold
 * an address, and are either typeless or cast as a pointer or
 * typed data; c) indeterminate size of at least 16 bits, in
 * whatever size provides the maximum speed (primary) and utility
 * (secondary); d) as above, but only 8 bits.
 *
 * The types provided in all cases are typeless, integer, unsigned and bit. 
 *
 * Note that the fast data types are not convertible! If you need to
 * coerce something, use a specified length.
 *
 * This file is for the PDP-11; note that the 11 doesn't have unsigned
 * bytes or longs; you always get signed. Sigh...
 */


/* Typeless of specified length */

#define	byte	char			/* Byte, no type data */
#define	swrd	uns			/* Single version */
#define	lwrd	long			/* And long */


/* Type and length specified */

#define	bitb	char			/* Binary byte */
#define	bits	uns			/* Short */
#define	bitl	long			/* Long */
#define	unsb	char			/* Unsigned byte */
#define	unss	uns			/* Short */
#define	unsl	long			/* Long */
#define	intb	char			/* Signed byte */
#define	ints	int			/* Short */
#define	intl	long			/* Long */


/* Define word type, both untyped and typed */

#define	word	uns			/* Machine word */
#define	bitw	uns			/* Word bit */
#define	unsw	uns			/* Unsigned */
#define	intw	int			/* Integer */


/* Fast and tiny data types */

#define	fwrd	uns			/* Typeless */
#define	bitf	uns			/* Bit */
#define	unsf	uns			/* Unsigned */
#define	intf	int			/* Integer */

#define	twrd	char			/* Typeless */
#define	bitt	char			/* Bit */
#define	unst	char			/* Not really unsigned */
#define	intt	char			/* Integer */


/* These definitions are per-machine because the compilers vary.
 * Void is defined as an int for machine which don't have the void
 * type, and routines are expected to return an int unless specified.
 * xreg is just like reg, except that it only has an effect on machines
 * with lots of registers, which can hold longs.
 */

#define	void	int
#define	xreg
