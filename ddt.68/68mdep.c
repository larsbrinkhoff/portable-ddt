#
/****************************************************************************

	DEBUGGER - 68000 instruction printout

****************************************************************************/

#ifndef DDTCAT	/* don't reget headers if catting when all routines internal */
#include	<pdefs.h>
#include	<ptypes.h>
#include	"mdep.h"
#include	"ddt.h"
#endif

static short	radix;
static unsl	dot;
static ddtst	*ldsp;

static int	dotinc;

static char *badop = "\t???";
static char *IMDF;				/* immediate data format */

static char *bname[16] = { "ra", "sr", "hi", "ls", "cc", "cs", "ne",
		    "eq", "vc", "vs", "pl", "mi", "ge", "lt", "gt", "le" };

static char *shro[4] = { "as", "ls", "rox", "ro" };

static char *bit[4] = { "btst", "bchg", "bclr", "bset" };

static int omove(),obranch(),oimmed(),oprint(),oneop(),soneop(),oreg(),ochk();
static int olink(),omovem(),oquick(),omoveq(),otrap(),oscc(),opmode(),shroi();
static int extend(),biti();

static struct opdesc
{			
	unsigned short mask, match;
	int (*opfun)();
	char *farg;
} opdecode[] =
{					/* order is important below */
  0xF000, 0x1000, omove, "b",		/* move instructions */
  0xF000, 0x2000, omove, "l",
  0xF000, 0x3000, omove, "w",
  0xF000, 0x6000, obranch, 0,		/* branches */
  0xFF00, 0x0000, oimmed, "or",		/* op class 0  */
  0xFF00, 0x0200, oimmed, "and",
  0xFF00, 0x0400, oimmed, "sub",
  0xFF00, 0x0600, oimmed, "add",
  0xFF00, 0x0A00, oimmed, "eor",
  0xFF00, 0x0C00, oimmed, "cmp",
  0xF100, 0x0100, biti, 0,
  0xF800, 0x0800, biti, 0,
  0xFFC0, 0x40C0, oneop, "move_from_sr", /* op class 4 */
  0xFF00, 0x4000, soneop, "negx",
  0xFF00, 0x4200, soneop, "clr",
  0xFFC0, 0x44C0, oneop, "move_to_ccr",
  0xFF00, 0x4400, soneop, "neg",
  0xFFC0, 0x46C0, oneop, "move_to_sr",
  0xFF00, 0x4600, soneop, "not",
  0xFFC0, 0x4800, oneop, "nbcd",
  0xFFF8, 0x4840, oreg, "swap\td%D",
  0xFFC0, 0x4840, oneop, "pea",
  0xFFF8, 0x4880, oreg, "extw\td%D",
  0xFFF8, 0x48C0, oreg, "extl\td%D",
  0xFB80, 0x4880, omovem, 0,
  0xFFC0, 0x4AC0, oneop, "tas",
  0xFF00, 0x4A00, soneop, "tst",
  0xFFF0, 0x4E40, otrap, 0,
  0xFFF8, 0x4E50, olink, 0,
  0xFFF8, 0x4880, oreg, "extw\td%D",
  0xFFF8, 0x48C0, oreg, "extl\td%D",
  0xFFF8, 0x4E58, oreg, "unlk\ta%D",
  0xFFF8, 0x4E60, oreg, "move\ta%D,usp",
  0xFFF8, 0x4E68, oreg, "move\tusp,a%D",
  0xFFFF, 0x4E70, oprint, "reset",
  0xFFFF, 0x4E71, oprint, "nop",
  0xFFFF, 0x4E72, oprint, "stop",
  0xFFFF, 0x4E73, oprint, "rte",
  0xFFFF, 0x4E75, oprint, "rts",
  0xFFFF, 0x4E76, oprint, "trapv",
  0xFFFF, 0x4E77, oprint, "rtr",
  0xFFC0, 0x4E80, oneop, "jsr",
  0xFFC0, 0x4EC0, oneop, "jmp",
  0xF1C0, 0x4180, ochk, "chk",
  0xF1C0, 0x41C0, ochk, "lea",
  0xF0C0, 0x50C0, oscc, 0,
  0xF100, 0x5000, oquick, "addq",
  0xF100, 0x5100, oquick, "subq",
  0xF000, 0x7000, omoveq, 0,
  0xF1C0, 0x80C0, ochk, "divu",
  0xF1C0, 0x81C0, ochk, "divs",
  0xF1F0, 0x8100, extend, "sbcd",
  0xF000, 0x8000, opmode, "or",
  0xF1C0, 0x91C0, opmode, "sub",
  0xF130, 0x9100, extend, "subx",
  0xF000, 0x9000, opmode, "sub",
  0xF1C0, 0xB1C0, opmode, "cmp",
  0xF138, 0xB108, extend, "cmpm",
  0xF100, 0xB000, opmode, "cmp",
  0xF100, 0xB100, opmode, "eor",
  0xF1C0, 0xC0C0, ochk, "mulu",
  0xF1C0, 0xC1C0, ochk, "muls",
  0xF1F8, 0xC188, extend, "exg",
  0xF1F8, 0xC148, extend, "exg",
  0xF1F8, 0xC140, extend, "exg",
  0xF1F0, 0xC100, extend, "abcd",
  0xF000, 0xC000, opmode, "and",
  0xF1C0, 0xD1C0, opmode, "add",
  0xF130, 0xD100, extend, "addx",
  0xF000, 0xD000, opmode, "add",
  0xF100, 0xE000, shroi, "r",
  0xF100, 0xE100, shroi, "l",
  0, 0, 0, 0
};

unsb	md_prinst(dsp, addr)
reg	ddtst	*dsp;
	unsw	addr;
{
	unss	inst;
	register struct opdesc *p;
	register int (*fun)();

	d_mskbpts(dsp);		/* this will lose big in run mode */
	ldsp = dsp;
	radix = dsp->d_obase;
	dot = addr;
	inst = *(unss *)dot;
/*	dot += 2; */
	dotinc = 2;
	switch (radix) {
		case 8:
			IMDF = "#%O";
			break;
		case 10:
			IMDF = "#%D";
			break;
		default:
			IMDF = "#%X";
			break;
		}
	for (p = opdecode; p->mask; p++)
		if ((inst & p->mask) == p->match) break;
	if (p->mask != 0) {
		(*p->opfun)(inst, p->farg);
		d_unmskbpts(dsp);
		return dotinc;
	}
	else {
		printf(badop);
		d_unmskbpts(dsp);
		return 2;
	}
}

printea(mode,regstr,size)
long mode, regstr;
int size;
{
	long index,disp;

	switch ((int)(mode)) {
	  case 0:	printf("d%D",regstr);
			break;

	  case 1:	printf("a%D",regstr);
			break;

	  case 2:	printf("a%D@",regstr);
			break;

	  case 3:	printf("a%D@+",regstr);
			break;

	  case 4:	printf("a%D@-",regstr);
			break;

	  case 5:	printf("a%D@(%D.)",regstr,instfetch(2));
			break;

	  case 6:	index = instfetch(2);
			disp = (char)(index&0377);
			printf("a%d@(%d,%c%D%c)",regstr,disp,
			  (index&0100000)?'a':'d',(index>>12)&07,
			  (index&04000)?'l':'w');
			break;

	  case 7:	switch ((int)(regstr))
			{
			  case 0:	index = instfetch(2);
					printf("%x:w",index);
					break;

			  case 1:	index = instfetch(4);
			  		d_prloc(ldsp, index);
					break;

			  case 2:	printf("pc@(%D.)",instfetch(2));
					break;

			  case 3:	index = instfetch(2);
					disp = (char)(index&0377);
					printf("a%D@(%D,%c%D:%c)",regstr,disp,
        			        (index&0100000)?'a':'d',(index>>12)&07,
					(index&04000)?'l':'w');
					break;

			  case 4:	index = instfetch(size==4?4:2);
					printf(IMDF, index);
					break;

			  default:	printf("???");
					break;
			}
			break;

	  default:	printf("???");
	}
}

printEA(ea,size)
long ea;
int size;
{
	printea((ea>>3)&07,ea&07,size);
}

mapsize(inst)
long inst;
{
	inst >>= 6;
	inst &= 03;
	return((inst==0) ? 1 : (inst==1) ? 2 : (inst==2) ? 4 : -1);
}

char suffix(size)
register int size;
{
	return((size==1) ? 'b' : (size==2) ? 'w' : (size==4) ? 'l' : '?');
}

static omove(inst, s)
long inst;
char *s;
{
	int size;

	printf("mov%c\t",*s);
	size = ((*s == 'b') ? 1 : (*s == 'w') ? 2 : 4);
	printea((inst>>3)&07,inst&07,size);
	printf(",");
	printea((inst>>6)&07,(inst>>9)&07,size);
}

static obranch(inst,dummy)
long inst;
{
	long disp = inst & 0377;
	char *s; 

	s = "s ";
	if (disp > 127) disp |= ~0377;
	else if (disp == 0) { s = " "; disp = instfetch(2); }
	printf("b%s%s\t",bname[(int)((inst>>8)&017)],s);
	d_prloc(ldsp, disp + inkdot(2));
}

static oscc(inst,dummy)
long inst;
{
	printf("s%s\t",bname[(int)((inst>>8)&017)]);
	printea((inst>>3)&07,inst&07,1);
}

static biti(inst, dummy)
long inst;
{
	printf("%s\t", bit[(int)((inst>>6)&03)]);
	if (inst&0x0100) printf("d%D,", inst>>9);
	else { printf(IMDF, instfetch(2)); printf(","); }
	printEA(inst);
}

static opmode(inst,opcode)
long inst;
{
	register int opmode = (int)((inst>>6) & 07);
	register int regstr = (int)((inst>>9) & 07);
	int size;

	size = (opmode==0 || opmode==4) ?
		1 : (opmode==1 || opmode==3 || opmode==5) ? 2 : 4;
	printf("%s%c\t", opcode, suffix(size));
	if (opmode>=4 && opmode<=6)
	{
		printf("d%d,",regstr);
		printea((inst>>3)&07,inst&07, size);
	}
	else
	{
		printea((inst>>3)&07,inst&07, size);
		printf(",%c%d",(opmode<=2)?'d':'a',regstr);
	}
}

static shroi(inst,ds)
long inst;
char *ds;
{
	int rx, ry;
	char *opcode;
	if ((inst & 0xC0) == 0xC0)
	{
		opcode = shro[(int)((inst>>9)&03)];
		printf("%s%s\t", opcode, ds);
		printEA(inst);
	}
	else
	{
		opcode = shro[(int)((inst>>3)&03)];
		printf("%s%s%c\t", opcode, ds, suffix(mapsize(inst)));
		rx = (int)((inst>>9)&07); ry = (int)(inst&07);
		if ((inst>>5)&01) printf("d%d,d%d", rx, ry);
		else
		{
			printf(IMDF, (rx ? rx : 8));
			printf(",d%d", ry);
		}
	}
}		

static oimmed(inst,opcode) 
long inst;
register char *opcode;
{
	register int size = mapsize(inst);
	long const;

	if (size > 0)
	{
		const = instfetch(size==4?4:2);
		printf("%s%c\t", opcode, suffix(size));
		printf(IMDF, const); printf(",");
		printEA(inst,size);
	}
	else printf(badop);
}

static oreg(inst,opcode)
long inst;
register char *opcode;
{
	printf(opcode, (inst & 07));
}

static extend(inst, opcode)
long	inst;
char	*opcode;
{
	register int size = mapsize(inst);
	int ry = (inst&07), rx = ((inst>>9)&07);
	char c;

	c = ((inst & 0x1000) ? suffix(size) : ' ');
	printf("%s%c\t", opcode, c);
	if (*opcode == 'e')
	{
		if (inst & 0x0080) printf("d%D,a%D", rx, ry);
		else if (inst & 0x0008) printf("a%D,a%D", rx, ry);
		else printf("d%D,d%D", rx, ry);
	}
	else if ((inst & 0xF000) == 0xB000) printf("a%D@+,a%D@+", ry, rx);
	else if (inst & 0x8) printf("a%D@-,a%D@-", ry, rx);
	else printf("d%D,d%D", ry, rx);
}

static olink(inst,dummy)
long inst;
{
	printf("link\ta%D,", inst&07);
	printf(IMDF, instfetch(2));
}

static otrap(inst,dummy)
long inst;
{
	printf("trap\t");
	printf(IMDF, inst&017);
}

static oneop(inst,opcode)
long inst;
register char *opcode;
{
	printf("%s\t",opcode);
	printEA(inst);
}

pregmask(mask)
register int mask;
{
	register int i;
	register int flag = 0;

	printf("#<");
	for (i=0; i<16; i++)
	{
		if (mask&1)
		{
			if (flag) printf(","); else flag++;
			printf("%c%d",(i<8)?'d':'a',i&07);
		}
		mask >>= 1;
	}
	printf(">");
}

static omovem(inst,dummy)
long inst;
{
	register int i, list = 0, mask = 0100000;
	register int reglist = (int)(instfetch(2));

	if ((inst & 070) == 040)	/* predecrement */
	{
		for(i = 15; i > 0; i -= 2)
		{ list |= ((mask & reglist) >> i); mask >>= 1; }
		for(i = 1; i < 16; i += 2)
		{ list |= ((mask & reglist) << i); mask >>= 1; }
		reglist = list;
	}
	printf("movem%c\t",(inst&100)?'l':'w');
	if (inst&02000)
	{
		printEA(inst);
		printf(",");
		pregmask(reglist);
	}
	else
	{
		pregmask(reglist);
		printf(",");
		printEA(inst);
	}
}

static ochk(inst,opcode)
long inst;
register char *opcode;
{
	printf("%s\t",opcode);
	printEA(inst);
	printf(",%c%D",(*opcode=='l')?'a':'d',(inst>>9)&07);
}

static soneop(inst,opcode)
long inst;
register char *opcode;
{
	register int size = mapsize(inst);

	if (size > 0)
	{
		printf("%s%c\t",opcode,suffix(size));
		printEA(inst);
	}
	else printf(badop);
}

static oquick(inst,opcode)
long inst;
register char *opcode;
{
	register int size = mapsize(inst);
	register int data = (int)((inst>>9) & 07);

	if (data == 0) data = 8;
	if (size > 0)
	{
		printf("%s%c\t", opcode, suffix(size));
		printf(IMDF, data); printf(",");
		printEA(inst);
	}
	else printf(badop);
}

static omoveq(inst,dummy)
long inst;
{
	register int data = (int)(inst & 0377);

	if (data > 127) data |= ~0377;
	printf("moveq\t"); printf(IMDF, data);
	printf(",d%D", (inst>>9)&07);
}

static oprint(inst,opcode)
long inst;
register char *opcode;
{
	printf("%s",opcode);
}


long
inkdot(n)
int n;
{
	return(dot+n);
}


long
instfetch(size)
int size;
{
	register long l1,l2;

	if (size == 4) {
		l1 = *(long *)(inkdot(dotinc));
		dotinc += 2;
	} else {
		l1 = (long)(*(short *)(inkdot(dotinc)));
		if (l1 & 0x8000) l1 |= 0xFFFF0000;
	}
	dotinc += 2;
	return(l1);
}

#define	JMASK	0xffc0
#define	JVAL	0x4e80
#define	BMASK	0xff00
#define	BVAL	0x6100

/* Returns 0 if instruction at addr can be ^X'd. */
intern unst	md_isxsi(dsp, addr)
reg	ddtst	*dsp;
unss	*addr;
{
	reg	swrd	inst;
		retcd	d_fetch();

	if (d_fetch(dsp, A_DBGI, sizeof(swrd), addr, &dsp->d_swrd) == ERR)
		ddtbug("no pc fetch");
	inst = dsp->d_swrd;

	if ((inst & BMASK) == BVAL) {
		if ((inst & 0xff) == 0) return 4;
		else return 2;
	}
	
	if ((inst & JMASK) == JVAL) {
		switch (inst & 070) {
		case 020: return 2;
		case 050:
		case 060: return 4;
		case 070:
			switch (inst & 07) {
			case 0: return 4;
			case 1: return 6;
			case 2: return 4;
			case 3: return 4;
			default: return 0;
			}
		default: return 0;
		}
	}
	
	return 0;
}
