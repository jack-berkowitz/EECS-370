	lw	0	1	value1
	noop
	noop
	noop
	lw	0	2	value2
	noop
	noop
	noop
	add	1	2	3
	noop
	noop
	noop
	lw	0	5	value3
	noop
	noop
	noop
	sw	5	3	0
	noop
	noop
	noop
	lw	5	6	0
	noop
	noop
	noop
	beq	3	6	skip
	noop
	noop
	noop
	add	0	0	0
skip	add	2	2	2
	halt
value1	.fill	10
value2	.fill	20
value3	.fill	100
