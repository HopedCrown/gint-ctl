.global _memory_read
.global _memory_write
.global _memory_dsp_xram_memset
.global _memory_dsp_yram_memset
.global _memory_dsp_xyram_memcpy

_memory_read:
	dt	r5
	bf/s	_memory_read
	mov.b	@r4+, r0

	rts
	nop

_memory_write:
	add	r5, r4

1:	dt	r5
	bf/s	1b
	mov.b	r0, @-r4

	rts
	nop

_memory_dsp_xram_memset:
	ldrs	1f
	ldre	1f
	shlr2	r5
	ldrc	r5
	mov	#0, r6
	lds	r6, x0

1:	movx.l	x0, @r4+

	rts
	nop

_memory_dsp_yram_memset:
	mov	r4, r2
	ldrs	1f
	ldre	1f
	shlr2	r5
	ldrc	r5
	mov	#0, r6
	lds	r6, y0

1:	movy.l	y0, @r2+

	rts
	nop

_memory_dsp_xyram_memcpy:
	ldrs	1f
	ldre	1f
	shlr2	r6
	add	#-1, r6
	ldrc	r6
	mov	r4, r7

	/* First load from XRAM */
	movx.w	@r5+, x0

	/* Write to YRAM then load from XRAM, in parallel */
1:	pcopy x0, a0 movx.w @r5+, x0 movy.w a0, @r7+

	/* Last write to YRAM */
	rts
	movy.w	a0, @r7+
