# Makefile for ddt

.SUFFIXES:	.b .o .lst .c .s .h .sh 
CC68 =	cc68 -DDDTLM -DDDTDBG -I/a/dab/cgw/qbus/ddt -c -O 

src = hcddt.c bpt.c hc.c parse.c cmd.c ops.c val.c util.c 68mdep.c 68lm.c \
	symt.c
obj = parse.b cmd.b val.b ops.b util.b bpt.b hcddt.b symt.b 68mdep.b 68lm.b \
	hc.b 68hc.b

lib:	../lib/libddt.a

../lib/libddt.a:	ddt.b 68hc.b isymt.b
	ar cr ../lib/libddt.a \
		68hc.b ddt.b 
#		68hc.b hc.b hcddt.b parse.b ops.b \
#		68mdep.b util.b symt.b cmd.b bpt.b 68lm.b val.b
	cp isymt.b ../lib/isymt.b

ddt.b:	ddt.c
	${CC68} ddt.c

ddt.c:	cat_defs ${src}
	cat cat_defs ${src} > ddt.c

isymt.b: isymt.c

tst:	tst.b ${obj} isymt.b
	ld68 -t -T 400 -o tst ../lib/crtddt.b tst.b ${obj} \
		../lib/libc.a -lc isymt.b
	ldldb tst tst.ldb -b 400

tst1:	tst.b ../lib/libddt.a isymt.b
	ld68 -t -T 400 -o tst ../lib/crtddt.b tst.b ../lib/libddt.a \
		../lib/libc.a -lc ../lib/isymt.b
	ldldb tst tst.ldb -b 400 -n

.c.b:
	${CC68} $*.c

.s.b:
	${CC68} $*.s

.c.s:
	${CC68} -S $*.c
