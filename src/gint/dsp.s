.global _dsp_ldrc
.global _dsp_padd
.global _dsp_cpumul
.global _dsp_loop

_dsp_ldrc:
	/* Check that LDRC is available */
	ldrc	#16
	rts
	nop

_dsp_padd:
	/* Add some numbers */
	lds	r4, x0
	lds	r5, y0
	padd	x0, y0, a0
	sts	a0, r0
	rts
	nop

_dsp_cpumul:
	/* Multiply stuff but use the CPU */
	lds	r4, x0
	lds	r5, y0

	/* Make it normal registers... (slow) */
	sts	x0, r0
	sts	y0, r1
	mul.l	r0, r1
	/* Take result back directly to DSP (latency?) */
	psts	macl, a0

	/* Return the result in r0 */
	sts	a0, r0
	rts
	nop

_dsp_loop:
	mov	#1, r0
	lds	r0, x0
	pclr	y0

	ldrs	1f
	ldre	2f
	ldrc	r4
	nop

1:
2:
	padd	x0, y0, y0

	sts	y0, r0
	rts
	nop
