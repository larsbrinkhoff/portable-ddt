| ddtstart.s
|
|
| Assembly language assist and startup for MC68000 DDT.  Get a
| stack, initialize memory management, vectors, and I/O, then
| go to main routine.  Also contains trap handling routines and
| return to interrupted program.
|


SSP	=	15 * 4		| offset of saved SP
SSR	=	16 * 4		| offset of saved SR
SPC	=	17 * 4		| offset of saved PC from start of save area


NXMVEC	=	2 * 4		| nonexistent memory vector
ODDVEC	=	3 * 4		| odd address vector
ILLVEC	=	4 * 4		| illegal instr. vector
UN1VEC	=	10 * 4		| unimplemented instr. 1 vector
UN2VEC	=	11 * 4		| unimplemented instr. 2 vector
BPTVEC	=	47 * 4		| breakpoint trap vector
TRCVEC	=	9 * 4		| trace trap vector
MAXVEC	=	255		| 255 vectors maximum


	.globl	DDT, ddtstack, ddtregs, ddtexec

	.globl	nxmaddr		| address where nxm information is saved

|	Saved SR, PC, and symbol table
	.globl	ddtps,ddtpc

|	Calls out
	.globl	ddthc		| initialization
	.globl	ddtrte		| run time errors
	.globl	ddtbpt		| breakpoints
	.globl	ddttrc		| trace traps

DDT:				| Main DDT entry point
	movw	#0x2700,sr	| disable interrupts, supervisor mode
	movl	#ddtstack,sp	| get our own stack
	jsr	inivec		| init. interrupt vectors
	jsr	ddthc		| goto debugger
	bra	rtn		| should never return

| Breakpoint Trap handler

bkpt_trap:
	movw	#0x2700,sr	| disable interrupts
	movw	sp@+,ddtregs+SSR+2 | copy SR to save structure
	clrw	ddtregs+SSR
	movl	sp@+,ddtregs+SPC | and PC also
	moveml	#0xFFFF,ddtregs | save all registers
	movl	#ddtstack,sp	| get our own stack
	jsr	ddtbpt		| go process trap
	bra	rtn		| shouldn't ever return


| Trace Trap handler

trc_trap:
	movw	#0x2700,sr	| disable interrupts
	movw	sp@+,ddtregs+SSR+2 | copy SR to save structure
	clrw	ddtregs+SSR
	movl	sp@+,ddtregs+SPC | and PC also
	moveml	#0xFFFF,ddtregs	| save all registers
	movl	#ddtstack,sp	| get our own stack
	jsr	ddttrc		| go process trap
	bra	rtn		| shouldn't ever return


| Unexpected Trap/Interrupt handler

bad_trap:
	movw	#0x2700,sr	| disable interrupts
	movw	sp@+,ddtregs+SSR+2 | copy SR to save structure
	clrw	ddtregs+SSR
	movl	sp@+,ddtregs+SPC | and PC also
	moveml	#0xFFFF,ddtregs	| save all registers
	movl	#ddtstack,sp	| get our own stack
	movl	#btm,sp@-	| push message
	jsr	ddtrte		| go process trap
	bra	rtn		| shouldn't ever return

	.data
btm:	.asciz	"Unexpected trap"
	.text

| Bus Error (NXM) and Odd Address Trap handler

nxm_trap:
	movw	#0x2700,sr	| disable interrupts
	tstw	sp@+		| function word
	movl	sp@+,nxmaddr	| save the address that trapped
	tstw	sp@+		| instruction register
	movw	sp@+,ddtregs+SSR+2 | copy SR to save structure
	clrw	ddtregs+SSR
	movl	sp@+,ddtregs+SPC | and PC also
	moveml	#0xFFFF,ddtregs	| save all registers
	movl	#ddtstack,sp	| get our own stack
	movl	#nxmm,sp@-	| push message
	jsr	ddtrte		| go process trap
	bra	rtn		| shouldn't ever return

	.data
	.even
nxmaddr: .long	0		| the address of a nxm is saved here
nxmm:	.asciz	"NXM/ODD"
	.text

| Illegal Instruction trap

ill_trap:
	movw	#0x2700,sr	| disable interrupts
	movw	sp@+,ddtregs+SSR+2 | copy SR to save structure
	clrw	ddtregs+SSR
	movl	sp@+,ddtregs+SPC | and PC also
	moveml	#0xFFFF,ddtregs	| save all registers
	movl	#ddtstack,sp	| get our own stack
	movl	#illm,sp@-	| push message
	jsr	ddtrte		| go process trap
	bra	rtn		| shouldn't ever return

	.data
illm:	.asciz	"Illegal instruction"
	.text


| Return to Interrupted Program

rtn:
	moveml	ddtregs,#0xFFFF | restore user's registers
	movl	ddtregs+SPC,sp@- | and pc
	movw	ddtregs+SSR+2,sp@- | and sr
	rte			| go back to user's program

| Initialize interrupt vectors

inivec:
	subl	a0,a0		| no !@#$%^* clearing of address registers!
	movl	#bad_trap,d0
	movl	#MAXVEC,d1
loop:	movl	d0,a0@+		| put it out there
	subql	#1,d1
	bge	loop
	movl	#nxm_trap,NXMVEC | set nxm trap vector
	movl	#nxm_trap,ODDVEC | set odd addr. trap vector
	movl	#ill_trap,ILLVEC | set illegal instr. trap vector
	movl	#ill_trap,UN1VEC | set unimpl. instr. trap vector
	movl	#ill_trap,UN2VEC | set unimpl. instr. trap vector
	movl	#bkpt_trap,BPTVEC | set up breakpoint trap vector
	movl	#trc_trap,TRCVEC | and trace trap vector
	rts

| Code to reset machine for $G.  Should probably do a real reset
| someday.

ddtexec:
	movl	#tempstack, ddtregs + SSP  | give the user a temporary stack
	rts

.data
.even
	. = .+4096
ddtstack:
	.long	0
	.long	0
tempstack:

| register save locations
ddtregs:
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
	.long	0
ddtps:	.long	0
ddtpc:	.long	0
