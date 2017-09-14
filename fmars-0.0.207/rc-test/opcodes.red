;redcode-94
;name opcode sum
;strat Test conversion of opcodes in core checksum routine.
;strat The checksum should be: $D7872928
;assert 1
	jmp	0,0
	mov	0,0
	add	0,0
	sub	0,0
	mul	0,0
	jmz	0,0
	div	0,0
	mod	0,0
	jmn	0,0
	djn	0,0
	cmp	0,0
	slt	0,0
	spl	0,0
	dat	0,0
	seq	0,0
	sne	0,0
	nop	0,0
	ldp	0,0
	stp	0,0
