/* Arrange for correct machine dependant file to be included.
 */

#ifdef	PDP11
#include	"11mdep.h"
#endif

#ifdef	MC68000
#include	"68mdep.h"
#endif


#ifndef	BPTTYP
struct x { ERROR ERROR; };	/* Not for a supported CPU type */
#endif

