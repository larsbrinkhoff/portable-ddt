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

