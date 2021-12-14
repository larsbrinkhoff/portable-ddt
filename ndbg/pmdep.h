/* Arrange for correct machinen dependant file to be included.
 */

#ifdef	PDP11
#include	"11mdep.h"
#endif

#ifndef	PSTYP
struct x { ERROR ERROR; };	/* Not for a supported CPU type */
#endif
