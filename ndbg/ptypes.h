/* Arrange for correct types file to be included.
 */

#ifdef	PDP11
#include	"11types.h"
#endif

#ifdef	MC68000
#include	"68types.h"
#endif


#ifndef	byte
struct x { ERROR ERROR; };	/* Not for a supported CPU type */
#endif
