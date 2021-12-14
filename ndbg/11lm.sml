
; THESE DEFINES DESCRIBE THE DDT CONFIGURATION


	.GLOBL	LL		; SYMBOL FOR DEFAULT LOAD LOCATION


DDTDBG	=	1		; Turn on to get debugging halt

D.TTY	=	177560		; Address of console

LSI11	=	1		; different instructions if LSI11 (1 = true)
P1120	=	0		; has RTT, etc

LL	=	160000		; use entry point which prompts for file name
