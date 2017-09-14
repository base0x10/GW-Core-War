;redcode-94
;name default addressing modes, data fields and modifiers.
;strat If some parts of an instruction aren't given then
;strat pMARS defaults them.  The defaults are compatible
;strat with ICWS '88 standard redcode.
;assert MAXLENGTH >= 380
	mov	0 , 1
	mov	0 , $ 1
	mov	0 , # 1
	mov	0 , @ 1
	mov	0 , * 1
	mov	$ 0 , 1
	mov	$ 0 , $ 1
	mov	$ 0 , # 1
	mov	$ 0 , @ 1
	mov	$ 0 , * 1
	mov	# 0 , 1
	mov	# 0 , $ 1
	mov	# 0 , # 1
	mov	# 0 , @ 1
	mov	# 0 , * 1
	mov	@ 0 , 1
	mov	@ 0 , $ 1
	mov	@ 0 , @ 1
	mov	@ 0 , * 1
	mov	@ 0 , # 1

	nop	1
	nop	$ 1
	nop	# 1
	nop	@ 1
	nop	* 1
	nop	0 , 1
	nop	0 , $ 1
	nop	0 , # 1
	nop	0 , @ 1
	nop	0 , * 1
	nop	$ 0 , 1
	nop	$ 0 , $ 1
	nop	$ 0 , # 1
	nop	$ 0 , @ 1
	nop	$ 0 , * 1
	nop	# 0 , 1
	nop	# 0 , $ 1
	nop	# 0 , # 1
	nop	# 0 , @ 1
	nop	# 0 , * 1
	nop	@ 0 , 1
	nop	@ 0 , $ 1
	nop	@ 0 , @ 1
	nop	@ 0 , * 1
	nop	@ 0 , # 1

	dat	1
	dat	$ 1
	dat	# 1
	dat	@ 1
	dat	* 1
	dat	0 , 1
	dat	0 , $ 1
	dat	0 , # 1
	dat	0 , @ 1
	dat	0 , * 1
	dat	$ 0 , 1
	dat	$ 0 , $ 1
	dat	$ 0 , # 1
	dat	$ 0 , @ 1
	dat	$ 0 , * 1
	dat	# 0 , 1
	dat	# 0 , $ 1
	dat	# 0 , # 1
	dat	# 0 , @ 1
	dat	# 0 , * 1
	dat	@ 0 , 1
	dat	@ 0 , $ 1
	dat	@ 0 , @ 1
	dat	@ 0 , * 1
	dat	@ 0 , # 1

	jmp	1
	jmp	$ 1
	jmp	# 1
	jmp	@ 1
	jmp	* 1
	jmp	0 , 1
	jmp	0 , $ 1
	jmp	0 , # 1
	jmp	0 , @ 1
	jmp	0 , * 1
	jmp	$ 0 , 1
	jmp	$ 0 , $ 1
	jmp	$ 0 , # 1
	jmp	$ 0 , @ 1
	jmp	$ 0 , * 1
	jmp	# 0 , 1
	jmp	# 0 , $ 1
	jmp	# 0 , # 1
	jmp	# 0 , @ 1
	jmp	# 0 , * 1
	jmp	@ 0 , 1
	jmp	@ 0 , $ 1
	jmp	@ 0 , @ 1
	jmp	@ 0 , * 1
	jmp	@ 0 , # 1

	spl	1
	spl	$ 1
	spl	# 1
	spl	@ 1
	spl	* 1
	spl	0 , 1
	spl	0 , $ 1
	spl	0 , # 1
	spl	0 , @ 1
	spl	0 , * 1
	spl	$ 0 , 1
	spl	$ 0 , $ 1
	spl	$ 0 , # 1
	spl	$ 0 , @ 1
	spl	$ 0 , * 1
	spl	# 0 , 1
	spl	# 0 , $ 1
	spl	# 0 , # 1
	spl	# 0 , @ 1
	spl	# 0 , * 1
	spl	@ 0 , 1
	spl	@ 0 , $ 1
	spl	@ 0 , @ 1
	spl	@ 0 , * 1
	spl	@ 0 , # 1

	jmz	0 , 1
	jmz	0 , $ 1
	jmz	0 , # 1
	jmz	0 , @ 1
	jmz	0 , * 1
	jmz	$ 0 , 1
	jmz	$ 0 , $ 1
	jmz	$ 0 , # 1
	jmz	$ 0 , @ 1
	jmz	$ 0 , * 1
	jmz	# 0 , 1
	jmz	# 0 , $ 1
	jmz	# 0 , # 1
	jmz	# 0 , @ 1
	jmz	# 0 , * 1
	jmz	@ 0 , 1
	jmz	@ 0 , $ 1
	jmz	@ 0 , @ 1
	jmz	@ 0 , * 1
	jmz	@ 0 , # 1

	jmn	0 , 1
	jmn	0 , $ 1
	jmn	0 , # 1
	jmn	0 , @ 1
	jmn	0 , * 1
	jmn	$ 0 , 1
	jmn	$ 0 , $ 1
	jmn	$ 0 , # 1
	jmn	$ 0 , @ 1
	jmn	$ 0 , * 1
	jmn	# 0 , 1
	jmn	# 0 , $ 1
	jmn	# 0 , # 1
	jmn	# 0 , @ 1
	jmn	# 0 , * 1
	jmn	@ 0 , 1
	jmn	@ 0 , $ 1
	jmn	@ 0 , @ 1
	jmn	@ 0 , * 1
	jmn	@ 0 , # 1


	add	0 , 1
	add	0 , $ 1
	add	0 , # 1
	add	0 , @ 1
	add	0 , * 1
	add	$ 0 , 1
	add	$ 0 , $ 1
	add	$ 0 , # 1
	add	$ 0 , @ 1
	add	$ 0 , * 1
	add	# 0 , 1
	add	# 0 , $ 1
	add	# 0 , # 1
	add	# 0 , @ 1
	add	# 0 , * 1
	add	@ 0 , 1
	add	@ 0 , $ 1
	add	@ 0 , @ 1
	add	@ 0 , * 1
	add	@ 0 , # 1

	sub	0 , 1
	sub	0 , $ 1
	sub	0 , # 1
	sub	0 , @ 1
	sub	0 , * 1
	sub	$ 0 , 1
	sub	$ 0 , $ 1
	sub	$ 0 , # 1
	sub	$ 0 , @ 1
	sub	$ 0 , * 1
	sub	# 0 , 1
	sub	# 0 , $ 1
	sub	# 0 , # 1
	sub	# 0 , @ 1
	sub	# 0 , * 1
	sub	@ 0 , 1
	sub	@ 0 , $ 1
	sub	@ 0 , @ 1
	sub	@ 0 , * 1
	sub	@ 0 , # 1

	mul	0 , 1
	mul	0 , $ 1
	mul	0 , # 1
	mul	0 , @ 1
	mul	0 , * 1
	mul	$ 0 , 1
	mul	$ 0 , $ 1
	mul	$ 0 , # 1
	mul	$ 0 , @ 1
	mul	$ 0 , * 1
	mul	# 0 , 1
	mul	# 0 , $ 1
	mul	# 0 , # 1
	mul	# 0 , @ 1
	mul	# 0 , * 1
	mul	@ 0 , 1
	mul	@ 0 , $ 1
	mul	@ 0 , @ 1
	mul	@ 0 , * 1
	mul	@ 0 , # 1


	seq	0 , 1
	seq	0 , $ 1
	seq	0 , # 1
	seq	0 , @ 1
	seq	0 , * 1
	seq	$ 0 , 1
	seq	$ 0 , $ 1
	seq	$ 0 , # 1
	seq	$ 0 , @ 1
	seq	$ 0 , * 1
	seq	# 0 , 1
	seq	# 0 , $ 1
	seq	# 0 , # 1
	seq	# 0 , @ 1
	seq	# 0 , * 1
	seq	@ 0 , 1
	seq	@ 0 , $ 1
	seq	@ 0 , @ 1
	seq	@ 0 , * 1
	seq	@ 0 , # 1


	sne	0 , 1
	sne	0 , $ 1
	sne	0 , # 1
	sne	0 , @ 1
	sne	0 , * 1
	sne	$ 0 , 1
	sne	$ 0 , $ 1
	sne	$ 0 , # 1
	sne	$ 0 , @ 1
	sne	$ 0 , * 1
	sne	# 0 , 1
	sne	# 0 , $ 1
	sne	# 0 , # 1
	sne	# 0 , @ 1
	sne	# 0 , * 1
	sne	@ 0 , 1
	sne	@ 0 , $ 1
	sne	@ 0 , @ 1
	sne	@ 0 , * 1
	sne	@ 0 , # 1


	slt	0 , 1
	slt	0 , $ 1
	slt	0 , # 1
	slt	0 , @ 1
	slt	0 , * 1
	slt	$ 0 , 1
	slt	$ 0 , $ 1
	slt	$ 0 , # 1
	slt	$ 0 , @ 1
	slt	$ 0 , * 1
	slt	# 0 , 1
	slt	# 0 , $ 1
	slt	# 0 , # 1
	slt	# 0 , @ 1
	slt	# 0 , * 1
	slt	@ 0 , 1
	slt	@ 0 , $ 1
	slt	@ 0 , @ 1
	slt	@ 0 , * 1
	slt	@ 0 , # 1

	djn	0 , 1
	djn	0 , $ 1
	djn	0 , # 1
	djn	0 , @ 1
	djn	0 , * 1
	djn	$ 0 , 1
	djn	$ 0 , $ 1
	djn	$ 0 , # 1
	djn	$ 0 , @ 1
	djn	$ 0 , * 1
	djn	# 0 , 1
	djn	# 0 , $ 1
	djn	# 0 , # 1
	djn	# 0 , @ 1
	djn	# 0 , * 1
	djn	@ 0 , 1
	djn	@ 0 , $ 1
	djn	@ 0 , @ 1
	djn	@ 0 , * 1
	djn	@ 0 , # 1

	mod	0 , 1
	mod	0 , $ 1
	mod	0 , # 1
	mod	0 , @ 1
	mod	0 , * 1
	mod	$ 0 , 1
	mod	$ 0 , $ 1
	mod	$ 0 , # 1
	mod	$ 0 , @ 1
	mod	$ 0 , * 1
	mod	# 0 , 1
	mod	# 0 , $ 1
	mod	# 0 , # 1
	mod	# 0 , @ 1
	mod	# 0 , * 1
	mod	@ 0 , 1
	mod	@ 0 , $ 1
	mod	@ 0 , @ 1
	mod	@ 0 , * 1
	mod	@ 0 , # 1

	div	0 , 1
	div	0 , $ 1
	div	0 , # 1
	div	0 , @ 1
	div	0 , * 1
	div	$ 0 , 1
	div	$ 0 , $ 1
	div	$ 0 , # 1
	div	$ 0 , @ 1
	div	$ 0 , * 1
	div	# 0 , 1
	div	# 0 , $ 1
	div	# 0 , # 1
	div	# 0 , @ 1
	div	# 0 , * 1
	div	@ 0 , 1
	div	@ 0 , $ 1
	div	@ 0 , @ 1
	div	@ 0 , * 1
	div	@ 0 , # 1

	ldp	0 , 1
	ldp	0 , $ 1
	ldp	0 , # 1
	ldp	0 , @ 1
	ldp	0 , * 1
	ldp	$ 0 , 1
	ldp	$ 0 , $ 1
	ldp	$ 0 , # 1
	ldp	$ 0 , @ 1
	ldp	$ 0 , * 1
	ldp	# 0 , 1
	ldp	# 0 , $ 1
	ldp	# 0 , # 1
	ldp	# 0 , @ 1
	ldp	# 0 , * 1
	ldp	@ 0 , 1
	ldp	@ 0 , $ 1
	ldp	@ 0 , @ 1
	ldp	@ 0 , * 1
	ldp	@ 0 , # 1

	stp	0 , 1
	stp	0 , $ 1
	stp	0 , # 1
	stp	0 , @ 1
	stp	0 , * 1
	stp	$ 0 , 1
	stp	$ 0 , $ 1
	stp	$ 0 , # 1
	stp	$ 0 , @ 1
	stp	$ 0 , * 1
	stp	# 0 , 1
	stp	# 0 , $ 1
	stp	# 0 , # 1
	stp	# 0 , @ 1
	stp	# 0 , * 1
	stp	@ 0 , 1
	stp	@ 0 , $ 1
	stp	@ 0 , @ 1
	stp	@ 0 , * 1
	stp	@ 0 , # 1
