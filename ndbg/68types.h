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
 * typed data; and c) indeterminate size, in whatever size provides
 * the maximum speed (primary) and utility (secondary), with
 * a reasonable length. The types provided in all cases are integer,
 * unsigned and bit.
 *
 * This file is for the Motorola 68000.
 */


/* Typeless of specified length */

#define	byte	uns char		/* Byte, no type data */
#define	swrd	uns short		/* Single version */
#define	lwrd	uns long		/* And long */


/* Type and length specified */

#define	bitb	char			/* Binary byte */
#define	bits	uns short		/* Short */
#define	bitl	uns long		/* Long */
#define	unsb	uns char		/* Unsigned byte */
#define	unss	uns short		/* Short */
#define	unsl	uns long		/* Long */
#define	intb	char			/* Signed byte */
#define	ints	short			/* Short */
#define	intl	long			/* Long */


/* Define word type, both untyped and typed */

#define	word	uns long		/* Machine word */
#define	bitw	uns long		/* Word bit */
#define	unsw	uns long		/* Unsigned */
#define	intw	int 			/* Integer */


/* Fast data type, word length on this machine */

#define	fwrd	uns			/* Size of fast data type */
#define	bitf	short			/* Word bit */
#define	unsf	uns			/* Unsigned */
#define	intf	short			/* Integer */

#define	twrd	char			/* Typeless */
#define	bitt	char			/* Bit */
#define	unst	uns char		/* Unsigned */
#define	intt	char			/* Integer */


/* These definitions are per-machine because the compilers vary.
 * Void is defined as an int for machine which don't have the void
 * type, and routines are expected to return an int unless specified.
 * xreg is just like reg, except that it only has an effect on machines
 * with lots of registers, which can hold longs.
 */

#define	void	int
#define	xreg	reg
