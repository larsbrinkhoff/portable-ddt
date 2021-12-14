/* $Header: inst.h,v 2.1 86/01/28 18:04:48 jnc R7-0 $ */

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


/* This file contains definitions for instruction decoding.
 */


/* Effective address types */

#define	MSK_EAR	07		/* Mask for effective address register */
#define	SHF_EAM	3		/* Shift for effective address mode */
#define	MSK_EAM	07		/* Mask for effective address mode */
#define	SHF_DR	9		/* Shift for destination register */
#define	MSK_DR	07		/* Mask for destination register */
#define	SHF_DM	6		/* Shift for destination mode */
#define	MSK_DM	07		/* Mask for destination mode */

#define	EA_DR	0		/* D register direct */
#define	EA_AR	1		/* A register direct */
#define	EA_IND	2		/* A register indirect */
#define	EA_AI	3		/* A register auto increment */
#define	EA_AD	4		/* A register auto decrement */
#define	EA_DSP	5		/* A register with displacement */
#define	EA_IDX	6		/* Register with index */
#define	EA_EXT	7		/* Extended */

#define	EAX_AS	0		/* Absolute short address */
#define	EAX_AL	1		/* Absolute long address */
#define	EAX_PCD	2		/* PC with displacement */
#define	EAX_PCI	3		/* PC with index */
#define	EAX_IMM	4		/* Immediate data */

#define	MSK_IRT	0x8000		/* Index Address/Data register */
#define	SHF_IRG	12		/* Shift for index register */
#define	MSK_IRG 0x7		/* Mask for index register */
#define	MSK_ILN	0x800		/* Index length */
#define	MSK_ID	0xFF		/* Index displacement */


/* Common OpMode field definitions */

#define	SHF_OPM	6		/* Shift for OpMode field */
#define	MSK_OPM	07		/* Mask for OpMode field */
#define	MSK_OD	04		/* Mask for OpMode direction */
#define	MSK_AR	03		/* Mask for OpMode destination address reg */
#define	VAL_AR	3		/* Value for OpMode destination address reg */

#define	OPM_DDB	0		/* Destination data reg, length byte */
#define	OPM_DDS	1		/* Destination data reg, length short */
#define	OPM_DDL	2		/* Destination data reg, length long */
#define	OPM_DAS	3		/* Destination addr reg, length short */
#define	OPM_DEB	4		/* Destination effective addr, length byte */
#define	OPM_DES	5		/* Destination effective addr, length short */
#define	OPM_DEL	6		/* Destination effective addr, length long */
#define	OPM_DAL	7		/* Destination addr reg, length long */


/* Common size field definitions */

#define	SHF_DSZ	6		/* Shift for common size field */
#define	MSK_DSZ	03		/* Mask for common size field */

#define	DSZ_B	0		/* Byte data */
#define	DSZ_S	1		/* Short data */
#define	DSZ_L	2		/* Long data */
#define	DSZ_ILG	3		/* Illegal, escape to other instructions */


/* Common register number field declarations */

#define	SHF_REG	9		/* Shift for common size field */
#define	MSK_REG	07		/* Mask for common size field */


/* Branch mask and type, 16 types of branches, loops, etc. "ra", branch
 * always, and "sr", branch subroutine, are handled specially in the
 * table.
 */

#define	SHF_BRT	8		/* Shift to get type out */
#define	MSK_BRT	0xF		/* Mask for branch type */
#define	MSK_BRO	0xFF		/* Mask for small displacement */

intern	char	*bname[16] =	{	"t",	/* true */
					"f",	/* false */
					"hi",	/* bhi */
					"ls",	/* bls */
					"hs",	/* bhs, also bcc */
					"lo",	/* blo, also bcs */
					"ne",	/* bne */
					"eq",	/* beq */
					"vc",	/* bvc */
					"vs",	/* bvs */
					"pl",	/* bpl */
					"mi",	/* bmi */
					"ge",	/* bge */
					"lt",	/* blt */
					"gt",	/* bgt */
					"le"	/* ble */
				};


/* The 4 types of bit instructions, mask and names */

#define	SHF_BIT	6		/* Shift for bit instructions */
#define	MSK_BIT	03		/* Mask for bit instructions */
#define	MSK_BSD	0x100		/* Static/dynamic bit operation */

intern	char	*bit[4] =	{	"btst\t",
					"bchg\t",
					"bclr\t",
					"bset\t"
				};


/* The 4 types of shifts and rotates. */

#define	SHF_MSR 9		/* Shift for memory shift instructions */
#define	MSK_MSR	03		/* Mask for memory shift instructions */
#define	SHF_RSR 3		/* Shift for register shift instructions */
#define	MSK_RSR	03		/* Mask for register shift instructions */
#define	MSK_SIR	0x20		/* Count/Register flag bit */
#define	MSK_SDR	0x20		/* Direction flag bit */

intern	char	*shro[4] =	{	"as",	/* arithmetic shift */
					"ls",	/* logical shift */
					"rox",	/* rotate extended */
					"ro"	/* rotate */
				};



/* Functions used to print the various flavors of instructions. */

intern	unst	md_osimple();
intern	unst	md_opmode();
intern	unst	md_oimmed();
intern	unst	md_oeareg();
intern	unst	md_soneop();
intern	unst	md_oneop();
intern	unst	md_oreg();
intern	unst	md_oiarg();
intern 	unst	md_omove();
intern	unst	md_obranch();
intern	unst	md_odbcc();
intern	unst	md_oscc();
intern	unst	md_biti();
intern	unst	md_shroi();
intern	unst	md_extend();
intern	unst	md_oquick();
intern	unst	md_omvq();
intern	unst	md_omvm();
intern	unst	md_oicsr();
intern	unst	md_omvcsr();
intern	unst	md_olink();
intern	unst	md_otrap();


/* Actual opcode decoding table. Note that the order is important below,
 * since some instructions are hidden 'inside' others in illegal values.
 */

#define	opdesc	struct	opdstr

opdesc	{	swrd	mask;		/* Mask field */
		swrd	match;		/* Match value in field */
		unst	(* opfun)();	/* Routine to print instrs */
		char	*farg;		/* String for use in subr */
	};

intern	opdesc	opdecode[] = {	{ 0xF1C0, 0x3040, md_omove, "a.w\t" },
				{ 0xF1C0, 0x2040, md_omove, "a.l\t" },
				{ 0xF000, 0x1000, md_omove, ".b\t" },
				{ 0xF000, 0x2000, md_omove, ".l\t" },
				{ 0xF000, 0x3000, md_omove, ".w\t" },
				{ 0xFF00, 0x6000, md_obranch, "ra" },
				{ 0xFF00, 0x6100, md_obranch, "sr" },
				{ 0xF000, 0x6000, md_obranch, NULL },
				{ 0xFFBF, 0x003C, md_oicsr, "ori\t#" },
				{ 0xFF00, 0x0000, md_oimmed, "ori" },
				{ 0xFFBF, 0x023C, md_oicsr, "andi\t#" },
				{ 0xFF00, 0x0200, md_oimmed, "andi" },
				{ 0xFF00, 0x0400, md_oimmed, "subi" },
				{ 0xFF00, 0x0600, md_oimmed, "addi" },
				{ 0xFFBF, 0x0A3C, md_oicsr, "eori\t#" },
				{ 0xFF00, 0x0A00, md_oimmed, "eori" },
				{ 0xFF00, 0x0C00, md_oimmed, "cmpi" },
				{ 0xF138, 0x0108, md_oiarg, "movep*\t" },
				{ 0xFF00, 0x0E00, md_oiarg, "moves*\t" },
				{ 0xF100, 0x0100, md_biti, NULL },
				{ 0xF800, 0x0800, md_biti, NULL },
				{ 0xFFC0, 0x40C0, md_oneop, "move\tsr," },
				{ 0xFF00, 0x4000, md_soneop, "negx" },
				{ 0xFFC0, 0x42C0, md_oneop, "move\tccr," },
				{ 0xFF00, 0x4200, md_soneop, "clr" },
				{ 0xFFC0, 0x44C0, md_omvcsr, ",ccr" },
				{ 0xFF00, 0x4400, md_soneop, "neg" },
				{ 0xFFC0, 0x46C0, md_omvcsr, ",sr" },
				{ 0xFF00, 0x4600, md_soneop, "not" },
				{ 0xFFC0, 0x4800, md_oneop, "nbcd\t" },
				{ 0xFFF8, 0x4840, md_oreg, "swap\td" },
				{ 0xFFC0, 0x4840, md_oneop, "pea\t" },
				{ 0xFFF8, 0x4880, md_oreg, "ext.w\td" },
				{ 0xFFF8, 0x48C0, md_oreg, "ext.l\td" },
				{ 0xFB80, 0x4880, md_omvm, NULL },
				{ 0xFFC0, 0x4AC0, md_oneop, "tas\t" },
				{ 0xFF00, 0x4A00, md_soneop, "tst" },
				{ 0xFFF0, 0x4E40, md_otrap, NULL },
				{ 0xFFF8, 0x4E50, md_olink, NULL },
				{ 0xFFF8, 0x4E58, md_oreg, "unlk\ta" },
				{ 0xFFF8, 0x4E60, md_osimple, "move*\t,usp" },
				{ 0xFFF8, 0x4E68, md_oreg, "move\tusp,a" },
				{ 0xFFFF, 0x4E70, md_osimple, "reset" },
				{ 0xFFFF, 0x4E71, md_osimple, "nop" },
				{ 0xFFFF, 0x4E72, md_oiarg, "stop\t#" },
				{ 0xFFFF, 0x4E73, md_osimple, "rte" },
				{ 0xFFFF, 0x4E74, md_oiarg, "rtd\t#" },
				{ 0xFFFF, 0x4E75, md_osimple, "rts" },
				{ 0xFFFF, 0x4E76, md_osimple, "trapv" },
				{ 0xFFFF, 0x4E77, md_osimple, "rtr" },
				{ 0xFFFE, 0x4E7A, md_oiarg, "movec*\t" },
				{ 0xFFFF, 0x4AFC, md_osimple, "ilg" },
				{ 0xFFC0, 0x4E80, md_oneop, "jsr\t" },
				{ 0xFFC0, 0x4EC0, md_oneop, "jmp\t" },
				{ 0xF1C0, 0x4180, md_oeareg, "chk\t" },
				{ 0xF1C0, 0x41C0, md_oeareg, "lea\t" },
				{ 0xF0F8, 0x50C8, md_odbcc, NULL },
				{ 0xF0C0, 0x50C0, md_oscc, NULL },
				{ 0xF100, 0x5000, md_oquick, "addq" },
				{ 0xF100, 0x5100, md_oquick, "subq" },
				{ 0xF000, 0x7000, md_omvq, NULL },
				{ 0xF1C0, 0x80C0, md_oeareg, "divu\t" },
				{ 0xF1C0, 0x81C0, md_oeareg, "divs\t" },
				{ 0xF1F0, 0x8100, md_extend, "sbcd" },
				{ 0xF000, 0x8000, md_opmode, "or" },
				{ 0xF0C0, 0x90C0, md_opmode, "suba" },
				{ 0xF130, 0x9100, md_extend, "subx" },
				{ 0xF000, 0x9000, md_opmode, "sub" },
				{ 0xF0C0, 0xB0C0, md_opmode, "cmpa" },
				{ 0xF138, 0xB108, md_extend, "cmpm" },
				{ 0xF100, 0xB000, md_opmode, "cmp" },
				{ 0xF100, 0xB100, md_opmode, "eor" },
				{ 0xF1C0, 0xC0C0, md_oeareg, "mulu\t" },
				{ 0xF1C0, 0xC1C0, md_oeareg, "muls\t" },
				{ 0xF1F8, 0xC188, md_extend, "exg" },
				{ 0xF1F8, 0xC148, md_extend, "exg" },
				{ 0xF1F8, 0xC140, md_extend, "exg" },
				{ 0xF1F0, 0xC100, md_extend, "abcd" },
				{ 0xF000, 0xC000, md_opmode, "and" },
				{ 0xF0C0, 0xD0C0, md_opmode, "adda" },
				{ 0xF130, 0xD100, md_extend, "addx" },
				{ 0xF000, 0xD000, md_opmode, "add" },
				{ 0xF100, 0xE000, md_shroi, "r" },
				{ 0xF100, 0xE100, md_shroi, "l" },
				{ 0, 0, ((unst (*) ()) NULL), NULL }
			};



/* Instruction types for 'execute step' test */

#define	JMASK	0xFFC0		/* Jump to subroutine */
#define	JVAL	0x4E80
#define	BMASK	0xFF00		/* Branch subroutine */
#define	BVAL	0x6100
