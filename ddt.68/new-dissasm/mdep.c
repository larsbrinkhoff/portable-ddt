/* $Header: mdep.c,v 2.1 86/01/28 18:04:01 jnc R7-0 $ */

/*  Copyright 1985 J. Noel Chiappa
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


/* This file contains all the routines that are dependant on a particular
 * architecture but not on the specific mode in which the debugger is
 * running (i.e. as a front end, in a memory mapped system, etc). It
 * contains the instruction decode routine and the routine to tell which
 * operations should be 'execute-stepped' over.
 *
 * The following instructions are not fully decoded yet:
 * movep; moves; movec; move ax,usp
 */


#ifndef DDTCAT	/* don't reget headers if catting when all routines internal */
#include	"inc/pinc.h"
#include	"dbg/src/src/mdep.h"
#include	"dbg/src/src/ddt.h"
#endif
#include	"dbg/src/68/inst.h"



/* Definitions */

#define	REGBS	8		/* Base to use for printing register numbers */


/* Macros to print the effective address portion of an instruction */

#define	md_prea(dsp, addr, inst, size)	md_eaprint(dsp, addr, ((intf)			((inst >> SHF_EAM) & MSK_EAM)), ((intf) (inst & MSK_EAR)), size)


/* Random static */

intern	char	*badop = "\t???";


/* Function definitions */

intern	retcd	d_fetch();



/* Disassemble and print MC68000 instructions. This code is
 * based on the instruction decoder written by Chris Terman at MIT.
 * Find instruction in table via mask and match, and call appropriate
 * routine for that format instruction. Each instruction must return
 * the lenght of any additional fields (over the original shortword
 * instruction).
 */

intern	unst	md_prinst(dsp, addr)
reg	ddtst	*dsp;
	unsw	addr;

{	reg	swrd	inst;
	reg	opdesc	*odp;
		unst	len;

	if (d_fetch(dsp, A_INST, sizeof(swrd), addr, &dsp->d_swrd) == ERR)
		return(ERR);

	len = sizeof(swrd);
	inst = dsp->d_swrd;
	addr += sizeof(swrd);

	for (odp = opdecode; (odp->mask != 0) ; odp++)
		if ((inst & odp->mask) == odp->match)
			break;

	if (odp->mask == 0) {
		(*dsp->d_puts)(badop);
		return(sizeof(swrd));
		}

	len += (*odp->opfun)(dsp, addr, inst, odp->farg);
	return(len);
}


/* Following group of utility routines prints effective addresses,
 * etc.
 *
 * First routine gets an instruction or immediate data short word into
 * the short temporary in the DDT state block.
 */

intern	retcd	md_instget(dsp, addr)
reg	ddtst	*dsp;
	unsw	addr;

{	if (d_fetch(dsp, A_INST, sizeof(swrd), addr, &dsp->d_swrd) == ERR) {
		d_err(dsp);
		return(ERR);
		}

	return(OK);
}


/* This routine gets an immediate data item into
 * the long temporary in the DDT state block. 
 * Because of the byte order on the 68000, you
 * can't simply clear the long temporary and copy
 * the data into it; things gets justified incorrectly.
 */

intern	unsf	md_immget(dsp, size, addr)
reg	ddtst	*dsp;
reg	unsf	size;
	unsw	addr;

{	if (size != sizeof(lwrd))
		size = sizeof(swrd);

	if (d_fetch(dsp, A_INST, size, addr, ((size == sizeof(lwrd)) ?
			&dsp->d_lwrd : &dsp->d_swrd)) == ERR) {
		d_err(dsp);
		return(ERR);
		}

	if (size != sizeof(lwrd))
		dsp->d_lwrd = ((lwrd) dsp->d_swrd);

	return(size);
}


/* Routine to print contents of index word. Word in question is
 * in short temporary in DDT state block. Used by routine below
 * which deals with standard 68000 effective address format.
 */

intern	md_doidx(dsp)
reg	ddtst	*dsp;

{	(*dsp->d_puts)(((dsp->d_swrd & MSK_IRT) != 0) ? ",a" : ",d");
	d_lprrdx(dsp, ((unsl) ((dsp->d_swrd >> SHF_IRG) & MSK_IRG)), REGBS);
	(*dsp->d_puts)(((dsp->d_swrd & MSK_ILN) != 0) ? ".l)" : ".w)");
}


/* This routine takes an effective address field in two parts (due
 * to pinhead reordering of subfields in some instructions) and prints
 * it. Returns the size (if any) of extension fields.
 *
 * Should really have a separate return value for error so that
 * callers can check and abort of this barfs.
 */

intern	unst	md_eaprint(dsp, addr, mode, regno, size)
reg	ddtst	*dsp;
	unsw	addr;
	intf	mode;
reg	intf	regno;
	intf	size;

{	switch (mode) {

	  case EA_DR:	(*dsp->d_putc)('d');
			d_lprrdx(dsp, ((unsl) regno), REGBS);
			return(0);

	  case EA_AR:	(*dsp->d_putc)('a');
			d_lprrdx(dsp, ((unsl) regno), REGBS);
			return(0);

	  case EA_IND:	(*dsp->d_puts)("(a");
			d_lprrdx(dsp, ((unsl) regno), REGBS);
			(*dsp->d_putc)(')');
			return(0);

	  case EA_AI:	(*dsp->d_puts)("(a");
			d_lprrdx(dsp, ((unsl) regno), REGBS);
			(*dsp->d_puts)(")+");
			return(0);

	  case EA_AD:	(*dsp->d_puts)("-(a");
			d_lprrdx(dsp, ((unsl) regno), REGBS);
			(*dsp->d_putc)(')');
			return(0);

	  case EA_DSP:	if (md_instget(dsp, addr) == ERR)
				return(ERR);
			d_prloc(dsp, ((unsw) dsp->d_swrd));
			(*dsp->d_puts)("(a");
			d_lprrdx(dsp, ((unsl) regno), REGBS);
			(*dsp->d_putc)(')');
			return(sizeof(swrd));

	  case EA_IDX:	if (md_instget(dsp, addr) == ERR)
				return(ERR);
			d_lprrdx(dsp, ((unsl) (dsp->d_swrd & MSK_ID)),
					dsp->d_obase);
			(*dsp->d_puts)("(a");
			d_lprrdx(dsp, ((unsl) regno), REGBS);
			md_doidx(dsp);
			return(sizeof(swrd));

	  case EA_EXT:	switch (regno) {

			  case EAX_AS:	if (md_instget(dsp, addr) == ERR)
						return(ERR);
					d_prloc(dsp, ((unsw) dsp->d_swrd));
					return(sizeof(swrd));

			  case EAX_AL:	if (d_fetch(dsp, A_INST, sizeof(lwrd),
						  addr, &dsp->d_lwrd) == ERR) {
						d_err(dsp);
						return(ERR);
						}
			  		d_prloc(dsp, ((unsw) dsp->d_lwrd));
					return(sizeof(lwrd));

			  case EAX_PCD:	if (md_instget(dsp, addr) == ERR)
						return(ERR);
					d_prloc(dsp, (addr + dsp->d_swrd));
					(*dsp->d_puts)("(pc)");
					return(sizeof(swrd));

			  case EAX_PCI:	if (md_instget(dsp, addr) == ERR)
						return(ERR);
					d_lprrdx(dsp, ((unsl) (dsp->d_swrd &
						MSK_ID)), dsp->d_obase);
					(*dsp->d_puts)("(pc");
					md_doidx(dsp);
					return(sizeof(swrd));

			  case EAX_IMM:	if ((size = md_immget(dsp, size, addr))
								== ERR)
						return(ERR);
					(*dsp->d_putc)('#');
			  		d_prloc(dsp, ((unsw) dsp->d_lwrd));
					return(size);

			  default:	(*dsp->d_puts)(badop);
					return(0);
			  };

		  default:	ddtbug("bad mode");
		  };
}


/* Reads canonical instruction size field and returns operand size
 * based on it. Returns ERR on bad length.
 */

intern	unsf	md_mapsize(inst)
	swrd	inst;

{	reg	unsf	size;

	size = ((inst >> SHF_DSZ) & MSK_DSZ);
	if (size == DSZ_B)
		return(sizeof(byte));
	if (size == DSZ_S)
		return(sizeof(swrd));
	if (size == DSZ_L)
		return(sizeof(lwrd));

	return(ERR);
}


/* Return the opcode suffix based on the size. String has '.' and tab
 * added on for ease of use.
 */

intern	char	*md_suffix(size)
reg	unsf	size;

{	if (size == sizeof(byte)) 
		return(".b\t");
	if (size == sizeof(swrd))
		return(".w\t");
	if (size == sizeof(lwrd))
		return(".l\t");

	return(".?\t");
}


/* Print name only, no operands.
 */

intern	unst	md_osimple(dsp, addr, inst, opcode)
	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
	char	*opcode;

{	(*dsp->d_puts)(opcode);
	return(0);
}


/* Instructions with an operation field and an EA field
 * e.g.: or, sub, suba, cmp, cmpa, eor, and, add, adda
 */

intern	unst	md_opmode(dsp, addr, inst, opcode)
reg	ddtst	*dsp;
	unsw	addr;
reg	swrd	inst;
	char	*opcode;

{	reg	unsf	opmode;
	reg	unsf	regstr;
	reg	unsf	size;
		unst	len;

	opmode = ((inst >> SHF_OPM) & MSK_OPM);
	regstr = ((inst >> SHF_REG) & MSK_REG);

	switch (opmode) {

	  case OPM_DDB:
	  case OPM_DEB:	size = sizeof(byte);
			break;

	  case OPM_DDS:
	  case OPM_DAS:
	  case OPM_DES:	size = sizeof(swrd);
			break;

	  case OPM_DDL:
	  case OPM_DAL:
	  case OPM_DEL:	size = sizeof(lwrd);
			break;
	  }

	(*dsp->d_puts)(opcode);
	(*dsp->d_puts)(md_suffix(size));

	if ((opmode & MSK_AR) == VAL_AR) {
		len = md_prea(dsp, addr, inst, size);
		(*dsp->d_puts)(",a");
		d_lprrdx(dsp, ((unsl) regstr), REGBS);
		return(len);
		}

	if ((opmode & MSK_OD) != 0) {
		(*dsp->d_putc)('d');
		d_lprrdx(dsp, ((unsl) regstr), REGBS);
		(*dsp->d_putc)(',');
		return(md_prea(dsp, addr, inst, size));
		}

	len = md_prea(dsp, addr, inst, size);
	(*dsp->d_puts)(",d");
	d_lprrdx(dsp, ((unsl) regstr), REGBS);
	return(len);
}


/* Immediate mode instructions:
 * e.g. ori, andi, subi, addi, eori, cmpi
 */

intern	unst	md_oimmed(dsp, addr, inst, opcode) 
reg	ddtst	*dsp;
	unsw	addr;
reg	swrd	inst;
	char	*opcode;

{	reg	unsf	size;
		unst	len;

	size = md_mapsize(inst);
	if (size == ERR) {
		(*dsp->d_puts)(badop);
		return(0);
		}
	if ((len = md_immget(dsp, size, addr)) == ERR)
		return(ERR);

	(*dsp->d_puts)(opcode);
	(*dsp->d_puts)(md_suffix(size));
	(*dsp->d_putc)('#');
	d_prloc(dsp, ((unsw) dsp->d_lwrd));
	addr += len;
	(*dsp->d_putc)(',');
	len += md_prea(dsp, addr, inst, size);

	return(len);
}


/* One register, one <ea>:
 * e.g. chk, lea, divu, divs
 */

intern	unst	md_oeareg(dsp, addr, inst, opcode)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
reg	char	*opcode;

{		unst	len;

	(*dsp->d_puts)(opcode);
	len = md_prea(dsp, addr, inst, ((unsf) sizeof(swrd)));
	(*dsp->d_putc)(',');
	(*dsp->d_putc)((*opcode == 'l') ? 'a' : 'd');
	d_lprrdx(dsp, ((unsl) ((inst >> SHF_REG) & MSK_REG)), REGBS);
	return(len);
}


/* Single operand, multiple sizes:
 * negx, clr, neg, not, tst
 */

intern	unst	md_soneop(dsp, addr, inst, opcode)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
reg	char	*opcode;

{	reg	unsf	size;

	size = md_mapsize(inst);
	if (size == ERR) {
		(*dsp->d_puts)(badop);
		return(0);
		}

	(*dsp->d_puts)(opcode);
	(*dsp->d_puts)(md_suffix(size));
	return(md_prea(dsp, addr, inst, ((unsf) 0)));
}


/* One operand instructions, fixed size:
 * e.g. nbcd, pea, tas, jsr, jmp
 */

intern	unst	md_oneop(dsp, addr, inst, opcode)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
	char	*opcode;

{	(*dsp->d_puts)(opcode);
	return(md_prea(dsp, addr, inst, ((unsf) 0)));
}


/* One register operations:
 * e.g. swap, ext, unlk
 */

intern	unst	md_oreg(dsp, addr, inst, opcode)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
	char	*opcode;

{	(*dsp->d_puts)(opcode);
	d_lprrdx(dsp, ((unsl) (inst & MSK_EAR)), REGBS);
	return(0);
}


/* Handles certain instructions with a single immediate operand:
 * e.g. stop, rtd
 */

intern	unst	md_oiarg(dsp, addr, inst, opcode)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
	char	*opcode;

{	if (md_instget(dsp, addr) == ERR)
			return(ERR);

	(*dsp->d_puts)(opcode);
	d_lprrdx(dsp, ((unsl) dsp->d_swrd), dsp->d_obase);
	return(sizeof(swrd));
}


/* Handles all normal move instructions.
 */

intern	unst	md_omove(dsp, addr, inst, slen)
reg	ddtst	*dsp;
	unsw	addr;
reg	swrd	inst;
	char	*slen;

{	reg	unsf	size;
		unst	len;

	if ((inst & 0x3000) ==  0x1000)
		size = sizeof(byte);
	if ((inst & 0x3000) ==  0x2000)
		size = sizeof(lwrd);
	if ((inst & 0x3000) ==  0x3000)
		size = sizeof(swrd);

	(*dsp->d_puts)("move");
	(*dsp->d_puts)(slen);
	len = md_prea(dsp, addr, inst, size);
	addr += len;
	(*dsp->d_putc)(',');
	len += md_eaprint(dsp, addr, ((intf) ((inst >> SHF_DM) & MSK_DM)),
		((intf) ((inst >> SHF_DR) & MSK_DR)), size);

	return(len);
}


/* Handles bra, bsr, and b<cc> family of instructions. Offset needs
 * to be sign extended; brach is relative to this instruction plus
 * a short.
 */

intern	unst	md_obranch(dsp, addr, inst, spcl)
reg	ddtst	*dsp;
	unsw	addr;
reg	swrd	inst;
	char	*spcl;

{		intb	dspf;
	reg	ints	disp;
		char	*slen; 
		unst	len;

	dspf = (inst & MSK_BRO);
	disp = dspf;

	if (disp == 0) {
		if (md_instget(dsp, addr) == ERR)
			return(ERR);
		slen = ".w\t";
		len = sizeof(swrd);
		disp = dsp->d_swrd;
		}
	  else {
		slen = ".b\t";
		len = 0;
		}

	(*dsp->d_putc)('b');
	if (spcl != NULL)
		(*dsp->d_puts)(spcl);
	  else
		(*dsp->d_puts)(bname[((inst >> SHF_BRT) & MSK_BRT)]);
	(*dsp->d_puts)(slen);
	d_prloc(dsp, ((unsw) (disp + addr)));

	return(len);
}


/* Handles db<cc> family of instructions. Offset needs
 * to be sign extended; branch is relative to this instruction
 * plus a short.
 */

intern	unst	md_odbcc(dsp, addr, inst, dummy)
reg	ddtst	*dsp;
	unsw	addr;
reg	swrd	inst;
	char	*dummy;

{	reg	ints	disp;
		char	*slen; 
		unst	len;

	if (md_instget(dsp, addr) == ERR)
		return(ERR);
	disp = dsp->d_swrd;

	(*dsp->d_puts)("db");
	(*dsp->d_puts)(bname[((inst >> SHF_BRT) & MSK_BRT)]);
	(*dsp->d_puts)("\td");
	d_lprrdx(dsp, ((unsl) (inst & MSK_EAR)), REGBS);
	(*dsp->d_putc)(',');
	d_prloc(dsp, ((unsw) (disp + addr)));

	return(sizeof(swrd));
}


/* Handles s<cc> family of instructions.
 */

intern	unst	md_oscc(dsp, addr, inst, dummy)
reg	ddtst	*dsp;
	unsw	addr;
reg	swrd	inst;
	char	*dummy;

{	(*dsp->d_putc)('s');
	(*dsp->d_puts)(bname[((inst >> SHF_BRT) & MSK_BRT)]);
	(*dsp->d_putc)('\t');

	return(md_prea(dsp, addr, inst, ((unsf) sizeof(byte))));
}


/* Handles btst, bchg, bclr, and bset instructions,
 * both static and dynamic.
 */

intern	unst	md_biti(dsp, addr, inst, dummy)
reg	ddtst	*dsp;
	unsw	addr;
reg	swrd	inst;
	char	*dummy;

{		unst	len;

	(*dsp->d_puts)(bit[((inst >> SHF_BIT) & MSK_BIT)]);

	if ((inst & MSK_BSD) != 0) {
		len = 0;
		(*dsp->d_putc)('d');
		d_lprrdx(dsp, ((unsl) ((inst >> SHF_REG) & MSK_REG)), REGBS);
		}
	  else {
		if (md_instget(dsp, addr) == ERR)
			return(ERR);
		len = sizeof(swrd);
		addr += sizeof(swrd);
 		(*dsp->d_putc)('#');
		d_lprrdx(dsp, ((unsl) dsp->d_swrd), dsp->d_obase);
		}

	(*dsp->d_putc)(',');
	len += md_prea(dsp, addr, inst, ((unsf) sizeof(byte)));
	return(len);
}


/* The rotary and shift instructions.
 */

intern	unst	md_shroi(dsp, addr, inst, dr)
reg	ddtst	*dsp;
	unsw	addr;
reg	swrd	inst;
	char	*dr;

{		unsf	regstr;
		unsf	cnt_reg;

	if (((inst >> SHF_DSZ) & MSK_DSZ) == DSZ_ILG) {
		(*dsp->d_puts)(shro[((inst >> SHF_MSR) & MSK_MSR)]);
		(*dsp->d_puts)(dr);
		(*dsp->d_puts)(".w\t");
		return(md_prea(dsp, addr, inst, ((unsf) sizeof(swrd))));
		}

	(*dsp->d_puts)(shro[((inst >> SHF_RSR) & MSK_RSR)]);
	(*dsp->d_puts)(dr);
	(*dsp->d_puts)(md_suffix(md_mapsize(inst)));

	regstr = (inst & MSK_EAR);
	cnt_reg = ((inst >> SHF_REG) & MSK_REG);
	if ((inst & MSK_SIR) != 0) {
		(*dsp->d_putc)('d');
		d_lprrdx(dsp, ((unsl) cnt_reg), REGBS);
		}
	  else {
		(*dsp->d_putc)('#');
		if (cnt_reg == 0)
			cnt_reg = 8;
		d_lprrdx(dsp, ((unsl) cnt_reg), dsp->d_obase);
		}

	(*dsp->d_puts)(",d");
	d_lprrdx(dsp, ((unsl) regstr), REGBS);

	return(0);
}


/* Some special case two-register instructions:
 * e.g. exg, cmpm, addx, subx, abcd, sbcd
 */

intern	unst	md_extend(dsp, addr, inst, opcode)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
	char	*opcode;

{	reg	unsf	size;
		unsf	ry;
		unsf	rx;
		unst	md_prdyop();

	size = md_mapsize(inst);
	rx = ((inst >> SHF_REG) & MSK_REG);
	ry = (inst & MSK_EAR);

	if (inst & 0x1000) {				/* cmpm, subx */
		(*dsp->d_puts)(opcode);
		(*dsp->d_puts)(md_suffix(size));
		}
	  else {
		(*dsp->d_puts)(opcode);
		(*dsp->d_putc)('\t');
		}

	if (*opcode == 'e') {				/* exg */
		if (inst & 0x0080)
			return(md_prdyop(dsp, "d", ",a", "", rx, ry));

		if (inst & 0x0008)
			return(md_prdyop(dsp, "a", ",a", "", rx, ry));

		return(md_prdyop(dsp, "d", ",d", "", rx, ry));
		}

	if ((inst & 0xF000) == 0xB000)			/* cmpm */
		return(md_prdyop(dsp, "(a", ")+,(a", ")+", ry, rx));

	if (inst & 0x8)					/* addx, abcd, sbcd */
		return(md_prdyop(dsp, "-(a", "),-(a", ")", ry, rx));

	return(md_prdyop(dsp, "d", ",d", "", ry, rx));
}


/* Routine used by above to print arguments.
 */

intern	unst	md_prdyop(dsp, stra, strb, strc, rega, regb)
reg	ddtst	*dsp;
	char	*stra, *strb, *strc;
	unsf	rega, regb;

{	(*dsp->d_puts)(stra);
	d_lprrdx(dsp, ((unsl) rega), REGBS);
	(*dsp->d_puts)(strb);
	d_lprrdx(dsp, ((unsl) regb), REGBS);
	(*dsp->d_puts)(strc);

	return(0);
}


/* Handles short quick instructions:
 * addq, subq
 */

intern	unst	md_oquick(dsp, addr, inst, opcode)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
reg	char	*opcode;

{	reg	unsf	size;
	reg	unss	data;

	size = md_mapsize(inst);

	if (size == 0) {
		(*dsp->d_puts)(badop);
		return(0);
		}

	data = ((inst >> 9) & 07);
	if (data == 0)
		data = 8;

	(*dsp->d_puts)(opcode);
	(*dsp->d_puts)(md_suffix(size));
	(*dsp->d_putc)('#');
	d_lprrdx(dsp, ((unsl) data), dsp->d_obase);
	(*dsp->d_putc)(',');
	return(md_prea(dsp, addr, inst, 0));
}


/* Does 'moveq' instruction.
 */

intern	unst	md_omvq(dsp, addr, inst, dummy)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
	char	*dummy;

{	reg	unsf	data;

	(*dsp->d_puts)("moveq\t#");

	data = (inst & 0177);
	if ((inst & 0200) != 0) {
		(*dsp->d_putc)('-');
		data++;
		}

	d_lprrdx(dsp, ((unsl) data), dsp->d_obase);
	(*dsp->d_puts)(",d");
	d_lprrdx(dsp, ((unsl) ((inst >> 9) & 07)), REGBS);

	return(0);
}


/* Handles 'movem' instructions. If in predecrement mode, the
 * register list must be reveresed.
 */

intern	unst	md_omvm(dsp, addr, inst,dummy)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;

{	reg	intf	i;
	reg	bits	mask;
	reg	bits	list;
	reg	bits	reglist;
		unst	len;

	if (md_instget(dsp, addr) == ERR)
		return(ERR);

	mask = 0100000;
	list = 0;
	reglist = dsp->d_swrd;
	len = sizeof(bits);
	addr += sizeof(bits);

	if ((inst & 070) == 040) {
		for (i = 15; i > 0; i -= 2) {
			list |= ((mask & reglist) >> i);
			mask >>= 1;
			}
		for (i = 1; i < 16; i += 2) {
			list |= ((mask & reglist) << i);
			mask >>= 1;
			}
		reglist = list;
		}

	(*dsp->d_puts)("movem.");
	(*dsp->d_putc)((inst & 100) ? 'l' : 'w');
	(*dsp->d_putc)('\t');

	if ((inst & 02000) == 0) {
		md_pregmask(dsp, reglist);
		(*dsp->d_putc)(',');
		len += md_prea(dsp, addr, inst, 0);
		return(len);
		}

	len += md_prea(dsp, addr, inst, 0);
	(*dsp->d_putc)(',');
	md_pregmask(dsp, reglist);
	return(len);
}


/* Print a register mask. Mask has been placed in d0 to a7
 * order already.
 */

intern	md_pregmask(dsp, mask)
reg	ddtst	*dsp;
reg	bits	mask;

{	reg	unsf	i;
	reg	unsf	flag;

	flag = 0;
	(*dsp->d_puts)("#<");

	for (i = 0; i < 16; i++) {
		if (mask & 1) {
			if (flag)
				(*dsp->d_putc)(',');
			  else
				flag++;
			(*dsp->d_putc)((i < 8) ? 'd' : 'a');
			d_lprrdx(dsp, ((unsl) (i & 07)), REGBS);
			}
		mask >>= 1;
		}
	(*dsp->d_putc)('>');
}


/* Special immediate operand instructions with special register
 * destination:
 * e.g. ori ccr, ori sr, andi ccr, andi sr, eori ccr, eori sr
 */

intern	unst	md_oicsr(dsp, addr, inst, opcode)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
	char	*opcode;

{	if (md_instget(dsp, addr) == ERR)
			return(ERR);

	(*dsp->d_puts)(opcode);
	d_lprrdx(dsp, ((unsl) dsp->d_swrd), dsp->d_obase);
	if ((inst & 0x40) != 0)
		(*dsp->d_puts)(",sr");
	  else
		(*dsp->d_puts)(",ccr");

	return(sizeof(swrd));
}


/* Special one operand move instructions with special register
 * destination:
 * e.g. move to sr, move to ccr
 */

intern	unst	md_omvcsr(dsp, addr, inst, opcode)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
	char	*opcode;

{		unst	len;

	(*dsp->d_puts)("move\t");
	len = md_prea(dsp, addr, inst, sizeof(swrd));
	(*dsp->d_puts)(opcode);
	return(len);
}


/* 'link' instruction only.
 */

intern	unst	md_olink(dsp, addr, inst, dummy)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
	char	*dummy;

{	if (md_instget(dsp, addr) == ERR)
		return(ERR);

	(*dsp->d_puts)("link\ta");
	d_lprrdx(dsp, ((unsl) (inst & MSK_EAR)), REGBS);
	(*dsp->d_puts)(",#");
	d_lprrdx(dsp, ((unsl) dsp->d_swrd), dsp->d_obase);

	return(sizeof(swrd));
}


/* 'trap' instruction only.
 */

intern	unst	md_otrap(dsp, addr, inst, dummy)
reg	ddtst	*dsp;
	unsw	addr;
	swrd	inst;
	char	*dummy;

{	(*dsp->d_puts)("trap\t#");
	d_lprrdx(dsp, ((unsl) (inst & 0xF)), dsp->d_obase);

	return(0);
}



/* Returns 0 if instruction at addr can be ^X'd by single stepping,
 * otherwise returns the length of the instrcuction, so that breakpoints
 * can be placed following it.
 * Illegal modes return 0.
 */

intern	unst	md_isxsi(dsp, addr)
reg	ddtst	*dsp;
	PCTYP	addr;

{	reg	swrd	inst;

	if (d_fetch(dsp, A_DBGI, sizeof(swrd),
				(unsw) addr, (unsw) &dsp->d_swrd) == ERR)
		ddtbug("no pc fetch");
	inst = dsp->d_swrd;

	if ((inst & BMASK) == BVAL) {
		if ((inst & MSK_BRO) == 0)
			return(2 * sizeof(swrd));
		return(sizeof(swrd));
		}
	
	if ((inst & JMASK) != JVAL)
		return(0);

	switch ((inst >> SHF_EAM) &  MSK_EAM) {

	  default:	return(0);

	  case EA_IND:	return(sizeof(swrd));

	  case EA_DSP:
	  case EA_IDX:	return(2 * sizeof(swrd));

	  case EA_EXT:	switch (inst & MSK_EAR) {

		  case EAX_AS:	return(2 * sizeof(swrd));

		  case EAX_AL:	return(3 * sizeof(swrd));

		  case EAX_PCD:	return(2 * sizeof(swrd));

		  case EAX_PCI:	return(2 * sizeof(swrd));

		  default:	return(0);
		  }
	  }
}
