	lw	0	1	mcand
	lw	0	2	mplier
	lw	0	4	one
	add	0	0	3
	nor	2	2	5
mask	nor	4	4	6
	nor	5	6	6
	beq	6	0	skip
	add	3	1	3
skip	add	4	4	4
	add	1	1	1
	beq	4	0	end
	beq	0	0	mask
end	halt
one	.fill	1
mcand	.fill	6203
mplier	.fill	1429
