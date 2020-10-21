
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
# r5: Buffer size (multiple of 4)
# r6: Number of rounds
_cpucache_rounds:
	mov	r4, r1
	add	r5, r1
	add	#-1, r1
	mov	r4, r0

1:	mov.b	@r0+, r2
	mov.b	@r0+, r2
	mov.b	@r0+, r2
	cmp/ge	r1, r0
	bf	1b
	mov.b	@r0+, r2

	dt	r6
	bf	_cpucache_rounds

	rts
	nop
