/* Turn on memory management; maps:
 *
 *	   virtual		   physical	what
 *
 *	00000 - 003FF		FFC00 - FFFFF	trap vectors
 *	10000 - 1FFFF		10000 - 1FFFF	I/O
 *	30000 - 3FFFF		30000 - 3FFFF	sync program
 *	40000 - 4FFFF		40000 - 4FFFF	video controller
 *	C0000 - DFFFF		C0000 - DFFFF	bitmap
 *	E0400 - F7FFF		E0400 - F7FFF	emulator
 *	F8000 - FFBFC		F8000 - FFBFC	stack
 */

#include "nu.h"
#include "tk.h"

int	*Map = (int *)MMAP;

				/* Map a page: virtual to physical adr	*/
page(virt, phys)		/* set up currently for small pages	*/
 unsigned virt, phys;
{	int pageno, padr;

	pageno = (virt >> PSIZE) & 0x3FF; 
	padr = phys >> PSIZE;
	Map[pageno] = padr;
}

mmgt()
{	register long adr;

	for (adr=0; adr < 0x100000; adr += 0x400)
		page(adr, adr);

	page(0x00000L,	0xFFC00L,	0);	/* Trap vectors		*/
	*((short *) PNUMREG) = 0;		/* make us process 0	*/
	*((short *) MCTRL) = 0xB;		/* turn it all on.	*/
						/* God help us...	*/
		/* except parity, God doesn't believe in parity...	*/
}
