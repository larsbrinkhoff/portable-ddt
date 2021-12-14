#

/* This file contains all the routines that are dependant on a particular
 * arhcitecture but not on the specific mode in which the debugger is
 * running (i.e. as a front end, in a memory mapped system, etc). It
 * contains the instruction decode routine and the routine to tell which
 * operations should be 'execute-stepped' over.
 */


#ifndef	DDTCAT	/* Don't reget hdrs if catting when all routines internal */
#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"
#endif



/* Format for compressed PDP11 instruction information.
 */

#define	pdpii	struct	pdpiistr

pdpii	{	unss	pi_bot;		/* First legal value */
		byte	pi_typ;		/* Type info, see below */
		unsb	pi_loc;		/* Location of string */
	};


#define	PI_BTYP	0200			/* Byte intruction */

#define	PI_MTYP	0160			/* Machine types which have this */
#define	PIM_ALL	0000			/* All */
#define	PIM_40	0020			/* 11/40 and up */
#define	PIM_45	0040			/* 11/45 and other new ones */
#define	PIM_23	0060			/* 11/23 and 11/21 */

#define	PI_ATYP	017			/* Argument type */
#define	PIA_NA	000			/* None */
#define	PIA_ILG	001			/* No intruction, print as num */
#define	PIA_CC	002			/* Condition code bits */
#define	PIA_N	003			/* 3 bit numerical */
#define	PIA_DS	004			/* 8 bit displacement */
#define	PIA_RDD	005			/* Register, destination */
#define	PIA_NN	006			/* 6 bit numerical */
#define	PIA_SD	007			/* Source, destination */
#define	PIA_R	010			/* Register */
#define	PIA_RDS	011			/* Register, 6 bit displacement */
#define	PIA_NNN	012			/* 8 bit numerical */
#define	PIA_DD	013			/* Destination */
#define	PIA_DDR	014			/* Destination, register */

#define	PI_OFFS	0377			/* F(*&%*&(H PDP11 MOVB!!!!! */


/* External storage of table; remain globals cause in ASS */

ext	pdpii	ddtini[];
ext	char	ddtins[];


/* Information on PDP-11 instruction formats */

#define	PDPBS	8		/* Use to type all numbers in instrs */

#define	PI_BYT	0100000		/* Byte instruction */

#define	CC_C	01		/* C condition code */
#define	CC_V	02		/* V condition code */
#define	CC_Z	04		/* Z condition code */
#define	CC_N	010		/* N condition code */

#define	MSK_N	07			/* Mask for N */
#define	MSK_DS	0377			/* Mask for displacement */
#define	MSK_RXX	0700			/* Mask for register in Rxx */
#define	SHF_RXX	06			/* Shift to get register */
#define	MSK_XDS	077			/* Mask for displacement */
#define	MSK_DD	077			/* Mask for destination */
#define	MSK_NN	077			/* Mask for NN */
#define	MSK_SS	07700			/* Mask for source in SSxx */
#define	SHF_SS	06			/* Shift to get source */
#define	MSK_R	07			/* Mask for register */
#define	MSK_NNN	0377			/* Mask for NNN */

#define	EA_MSK	060			/* Effective address calculation */
#define	EA_SHF	04			/* and shift */
#define	EA_IND	010			/* Indirect bit */
#define	EA_REG	00			/* Register */
#define	EA_AI	01			/* Autoincrement */
#define	EA_AD	02			/* Autodecrement */
#define	EA_IDX	03			/* Index */
#define	EA_IMM	01			/* PC modes; immediate */
#define	EA_REL	03			/* Relative */

#define	SP	6			/* Stack pointer */
#define	PC	7			/* Program counter */


/* Instruction types for 'execute-step' test */

#define	TRPMSK	0177000			/* Note both TRAP and EMT here */
#define	TRPINST	0104000
#define	IOTMSK	0177777
#define	IOTINST	0000004
#define	JSRMSK	0177000
#define	JSRINST	04000


/* Useful macros */

#define	prbit(bit, chr)		if ((tmp & bit) != 0) (*dsp->d_putc)(chr)
#define	prsep()			(*dsp->d_puts)(&cmsp[0])


/* Random static */

intern	char	cmsp[] =	", ";	/* For separating args */



/* Disassemble and print PDP-11 instructions. This code is a C rewrite
 * of the DDT instruction typeout routine from the MACRO DDT.
 * Find instruction in table, print instruction name, find typ
 * of arguments and print.
 */

intern	unst	md_prinst(dsp, addr)
reg	ddtst	*dsp;
	unsw	addr;

{	reg	pdpii	*pdp;
		swrd	inst;
		unsb	typ;
		unst	isiz;
	reg	unss	tmp;
		char	*p;
		char	*end;
		retcd	d_fetch();
		unst	md_prea();
		
	if (d_fetch(dsp, A_INST, sizeof(swrd), addr, &inst) != sizeof(swrd))
		return(ERR);

	isiz = sizeof(swrd);
	tmp = (inst & ~PI_BYT);
	pdp = &ddtini[0];

	for (;;) {
		while (tmp > pdp->pi_bot)
			pdp++;
		if (tmp != pdp->pi_bot)
			pdp--;

		if (tmp == inst)
			break;

		tmp = inst;
		if ((pdp->pi_typ & PI_BTYP) != 0)
			break;
		}

	p = (ddtins + (pdp->pi_loc & PI_OFFS));
	end = (ddtins + ((pdp + 1)->pi_loc & PI_OFFS));
	while (p < end)
		(*dsp->d_putc)(*p++);

	if (((pdp->pi_typ & PI_BTYP) != 0) && ((tmp & PI_BYT) != 0))
		(*dsp->d_putc)('B');

	typ = (pdp->pi_typ & PI_ATYP);

	if ((typ != PIA_NA) && (typ != PIA_CC))
		(*dsp->d_putc)('\t');

	switch(typ) {

	  case PIA_ILG:	d_lprrdx(dsp, ((unsl) tmp), PDPBS);
	  case PIA_NA:	break;

	  case PIA_CC:	prbit(CC_C, 'C');
			prbit(CC_V, 'V');
			prbit(CC_Z, 'Z');
			prbit(CC_N, 'N');
			break;

	  case PIA_N:	tmp &= MSK_N;
	  case PIA_NN:	tmp &= MSK_NN;
	  case PIA_NNN:	tmp &= MSK_NNN;
			d_lprrdx(dsp, ((unsl) tmp), PDPBS);
			break;

	  case PIA_DDR:	isiz += md_prea(dsp, addr, isiz, (tmp & MSK_DD));
			prsep();
	  case PIA_RDD:
	  case PIA_RDS:	md_prreg(dsp, ((tmp & MSK_RXX) >> SHF_RXX));
			if (typ == PIA_DDR)
				break;
			prsep();
			if (typ == PIA_RDD) {
		  		isiz += md_prea(dsp, addr, isiz,
							(tmp & MSK_DD));
				break;
				}
			tmp &= MSK_XDS;
			tmp = -tmp;		/* Offset backwards only */
	  case PIA_DS:	tmp = ((intb) (tmp & MSK_DS));
			tmp *= sizeof(swrd);
			tmp += (addr + sizeof(swrd));
			d_prloc(dsp, tmp);
			break;
			
	  case PIA_SD:	isiz += md_prea(dsp, addr, isiz,
						((tmp & MSK_SS) >> SHF_SS));
			prsep();
	  case PIA_DD:	isiz += md_prea(dsp, addr, isiz, (tmp & MSK_DD));
			break;

	  case PIA_R:	md_prreg(dsp, (tmp & MSK_R));
			break;

	  default:	ddtbug("bad operand type");
	  }

	return(isiz);
}


/* Print effective address */

intern	unst	md_prea(dsp, addr, offset, ea)
reg	ddtst	*dsp;
	unsw	addr;
	unst	offset;
reg	unsb	ea;

{	reg	unsb	eamode;
		swrd	tmp;
		retcd	d_fetch();
	
	eamode = ((ea & EA_MSK) >> EA_SHF);

	if ((ea & EA_IND) != 0)
		(*dsp->d_putc)('@');

	if ((ea & MSK_R) == PC) {

		if (eamode == EA_IMM) {
			if (d_fetch(dsp, A_INST, sizeof(swrd), (addr + offset),
							&tmp) == ERR) {
				d_err(dsp);
				return(0);
				}
			(*dsp->d_putc)('#');
			d_prloc(dsp, tmp);
			return(sizeof(swrd));
			}

		if (eamode == EA_REL) {
			if (d_fetch(dsp, A_INST, sizeof(swrd), (addr + offset),
							&tmp) == ERR) {
				d_err(dsp);
				return(0);
				}
			tmp += addr;
			tmp += (offset + sizeof(swrd));
			d_prloc(dsp, tmp);
			return(sizeof(swrd));
			}
		}
			
	switch(eamode) {

	  case EA_REG:	md_prreg(dsp, (ea & MSK_R));
			return(0);

	  case EA_AI:	(*dsp->d_putc)('(');
			md_prreg(dsp, (ea & MSK_R));
			(*dsp->d_puts)(")+");
			return(0);
			
	  case EA_AD:	(*dsp->d_puts)("-(");
			md_prreg(dsp, (ea & MSK_R));
			(*dsp->d_putc)(')');
			return(0);

	  case EA_IDX:	if (d_fetch(dsp, A_INST, sizeof(swrd), (addr + offset),
							&tmp) == ERR) {
				d_err(dsp);
				return(0);
				}
			d_prloc(dsp, tmp);
			(*dsp->d_putc)('(');
			md_prreg(dsp, (ea & MSK_R));
			(*dsp->d_putc)(')');
			return(sizeof(swrd));
	
	  default:	ddtbug("impossible ea mode");
	  }
}


/* Print register name.
 */

intern	md_prreg(dsp, regno)
reg	ddtst	*dsp;
	unst	regno;

{	if (regno > PC)
		ddtbug("illegal reg");

	if (regno == PC) {
		(*dsp->d_puts)("PC");
		return;
		}

	if (regno == SP) {
		(*dsp->d_puts)("SP");
		return;
		}

	(*dsp->d_putc)('R');
	(*dsp->d_putc)('0' + regno);
}


/* Is next instruction worth skipping over? If so, how long.
 * This routine is called only by the hardcore debugger.
 * In the auto-decrement case, insists on single stepping.
 */

intern	unst	md_isxsi(dsp, addr)
reg	ddtst	*dsp;
	swrd	*addr;

{	reg	swrd	inst;
		retcd	d_fetch();

	if (d_fetch(dsp, A_DBGI, sizeof(swrd), addr, &dsp->d_swrd) == ERR)
		ddtbug("no pc fetch");
	inst = dsp->d_swrd;

	if ((inst & TRPMSK) == TRPINST)
		return(sizeof(swrd));

	if ((inst & IOTMSK) == IOTINST)
		return(sizeof(swrd));

	if ((inst & JSRMSK) != JSRINST)
		return(0);

	switch((inst & EA_MSK) >> EA_SHF) {

	  case EA_REG:	return(sizeof(swrd));

	  case EA_AI:	if ((inst & MSK_R) == PC)
				return(2 * sizeof(swrd));
			return(sizeof(swrd));

	  case EA_AD:	if ((inst & MSK_R) == PC)
				return(0);
			return(sizeof(swrd));


	  case EA_IDX:	return(2 * sizeof(swrd));

	  }
}
