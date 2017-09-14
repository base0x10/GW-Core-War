;redcode-94
;name mode test
;strat Test addressing mode conversion in core checksum routine
;strate The checksum should be: $19FF5742
;assert 1

	jmp	#0,0
	jmp	$0,0
	jmp	@0,0
	jmp	<0,0
	jmp	>0,0
	jmp	*0,0
	jmp	{0,0
	jmp	}0,0
	jmp	0,#0
	jmp	0,$0
	jmp	0,@0
	jmp	0,<0
	jmp	0,>0
	jmp	0,*0
	jmp	0,{0
	jmp	0,}0
	
