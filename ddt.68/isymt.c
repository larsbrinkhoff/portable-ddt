extern	int	ddtregs[];

/* register names */
static char rega0[] = "a0";
static char rega1[] = "a1";
static char rega2[] = "a2";
static char rega3[] = "a3";
static char rega4[] = "a4";
static char rega5[] = "a5";
static char rega6[] = "a6";
static char rega7[] = "sp";
static char regd0[] = "d0";
static char regd1[] = "d1";
static char regd2[] = "d2";
static char regd3[] = "d3";
static char regd4[] = "d4";
static char regd5[] = "d5";
static char regd6[] = "d6";
static char regd7[] = "d7";
static char regsr[] = "sr";
static char regpc[] = "pc";

struct symt {
	char	*name;
	int	*value;
	};

struct symt D_ISYMT[] = {
	{ regd0, &ddtregs[0] },
	{ regd1, &ddtregs[1] },
	{ regd2, &ddtregs[2] },
	{ regd3, &ddtregs[3] },
	{ regd4, &ddtregs[4] },
	{ regd5, &ddtregs[5] },
	{ regd6, &ddtregs[6] },
	{ regd7, &ddtregs[7] },
	{ rega0, &ddtregs[8] },
	{ rega1, &ddtregs[9] },
	{ rega2, &ddtregs[10] },
	{ rega3, &ddtregs[11] },
	{ rega4, &ddtregs[12] },
	{ rega5, &ddtregs[13] },
	{ rega6, &ddtregs[14] },
	{ rega7, &ddtregs[15] },
	{ regsr, &ddtregs[16] },
	{ regpc, &ddtregs[17] } };
