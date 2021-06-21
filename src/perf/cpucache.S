
.global _cpucache_nop1024
.global _cpucache_rounds

#define r4(x) x; x; x; x

_cpucache_nop1024:
	r4(r4(r4(r4(r4(nop)))))

	dt	r4
	bt	1f
	bra	_cpucache_nop1024
	nop

1:	rts
	nop


# r4: Buffer to read from
# r5: Buffer size (multiple of 32)
# r6: Number of rounds
_cpucache_rounds:
	mov	r4, r0
	mov	r5, r2
	mov	#-5, r3
	shld	r3, r2

	ldrs	1f
	ldre	2f
	ldrc	r2
	nop

1:	mov.l	@r0+, r1
	mov.l	@r0+, r1
	mov.l	@r0+, r1
	mov.l	@r0+, r1
	mov.l	@r0+, r1
	mov.l	@r0+, r1
	mov.l	@r0+, r1
2:	mov.l	@r0+, r1

	dt	r6
	bf	_cpucache_rounds

	rts
	nop
