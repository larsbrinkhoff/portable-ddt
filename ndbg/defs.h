/* Short names and things to make code cleaner. The void business
 * is because this C compiler doesn't have it, and defaults things
 * to int.
 */

#define	reg	register
#define	ext	extern
#define	uns	unsigned
#define	dbl	double
#define	void	int


/* Things below are for use in portable code - provide rough PDP11
 * equivalents in cases where the exact types don't exist.
 */

#define	byte	char			/* Byte, no type data */
#define	word	unsigned		/* Word version */

#define	bit	uns			/* Bit field bit type */

#define	bitb	char			/* Binary byte */
#define	bitw	int			/* Word */
#define	bitl	long			/* Long */
#define	unsb	char			/* Unsigned byte (not really) */
#define	unsw	uns			/* Word */
#define	unsl	long			/* Long (not really) */
#define	intb	char			/* Signed byte */
#define	intw	int			/* Word */
#define	intl	long			/* Long */
