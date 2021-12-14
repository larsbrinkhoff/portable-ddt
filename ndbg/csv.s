/ C register save and restore -- version 12/74
/ Hacked to work with DDT so breakpoints can go in regular one

.globl	ddtcsv
.globl	ddtcret

ddtcsv:
	mov	r5,r0
	mov	sp,r5
	mov	r4,-(sp)
	mov	r3,-(sp)
	mov	r2,-(sp)
	jsr	pc,(r0)		/ jsr part is sub $2,sp

ddtcret:
	mov	r5,r2
	mov	-(r2),r4
	mov	-(r2),r3
	mov	-(r2),r2
	mov	r5,sp
	mov	(sp)+,r5
	rts	pc
