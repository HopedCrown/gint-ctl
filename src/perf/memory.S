/* Useful macros to get a repeating DSP loop for instances with more than
   4096 iterations. Uses r1 and r2, labels 3: and 4:. */
#define PRELUDE(ITERATIONS)		\
	mov.l	.l4095, r1;		\
	mov	#0, r0;			\
3:	ldrs	1f;			\
	cmp/hi	r1, ITERATIONS;		\
	ldre	2f;			\
	mov	r1, r2;			\
	bt	4f;			\
	mov	ITERATIONS, r2;		\
4:	ldrc	r2;			\
	nop
#define EPILOGUE(ITERATIONS)		\
	sub	r2, ITERATIONS;		\
	cmp/pl	ITERATIONS;		\
	bt	3b;			\
	rts;				\
	nop

/* Memory reads from CPU */

.global _mem_read8
.global _mem_read16
.global _mem_read32

_mem_read8:
	PRELUDE(r5)
1: 2:	mov.b	@r4+, r0
	EPILOGUE(r5)

_mem_read16:
	shlr	r5
	PRELUDE(r5)
1: 2:	mov.w	@r4+, r0
	EPILOGUE(r5)

_mem_read32:
	shlr2	r5
	PRELUDE(r5)
1: 2:	mov.l	@r4+, r0
	EPILOGUE(r5)

/* Memory writes from CPU */

.global _mem_write8
.global _mem_write16
.global _mem_write32

_mem_write8:
	add	r5, r4
	mov	#0, r0
	PRELUDE(r5)
1: 2:	mov.b	r0, @-r4
	EPILOGUE(r5)

_mem_write16:
	add	r5, r4
	mov	#0, r0
	shlr	r5
	PRELUDE(r5)
1: 2:	mov.w	r0, @-r4
	EPILOGUE(r5)

_mem_write32:
	add	r5, r4
	mov	#0, r0
	shlr2	r5
	PRELUDE(r5)
1: 2:	mov.l	r0, @-r4
	EPILOGUE(r5)

/* Alternate reads from CPU */

.global _mem_read8_alt
.global _mem_read16_alt
.global _mem_read32_alt

_mem_read8_alt:
	shlr	r6
	PRELUDE(r6)
1:	mov.b	@r4, r0
2:	mov.b	@r5, r0
	EPILOGUE(r6)

_mem_read16_alt:
	shlr2	r6
	PRELUDE(r6)
1:	mov.w	@r4, r0
2:	mov.w	@r5, r0
	EPILOGUE(r6)

_mem_read32_alt:
	shlr	r6
	shlr2	r6
	PRELUDE(r6)
1:	mov.l	@r4, r0
2:	mov.l	@r5, r0
	EPILOGUE(r6)

/* Alternate writes from CPU */

.global _mem_write8_alt
.global _mem_write16_alt
.global _mem_write32_alt

_mem_write8_alt:
	mov.b	@r4, r0
	mov.b	@r5, r3
	shlr	r6
	mov	#0, r0
	PRELUDE(r6)
1:	mov.b	r0, @r4
2:	mov.b	r3, @r5
	EPILOGUE(r6)

_mem_write16_alt:
	mov.w	@r4, r0
	mov.w	@r5, r3
	shlr2	r6
	PRELUDE(r6)
1:	mov.w	r0, @r4
2:	mov.w	r3, @r5
	EPILOGUE(r6)

_mem_write32_alt:
	mov.l	@r4, r0
	mov.l	@r5, r3
	shlr	r6
	shlr2	r6
	PRELUDE(r6)
1:	mov.l	r0, @r4
2:	mov.l	r3, @r5
	EPILOGUE(r6)

/* Memory reads and writes from DSP XRAM */

.global _mem_dspx_read16
.global _mem_dspx_read32
.global _mem_dspx_write16
.global _mem_dspx_write32

_mem_dspx_read16:
	shlr	r5
	PRELUDE(r5)
1: 2:	movx.w	@r4+, x0
	EPILOGUE(r5)

_mem_dspx_read32:
	shlr2	r5
	PRELUDE(r5)
1: 2:	movx.l	@r4+, x0
	EPILOGUE(r5)

_mem_dspx_write16:
	shlr	r5
	mov	#0, r0
	lds	r0, x0
	PRELUDE(r5)
1: 2:	movx.w	x0, @r4+
	EPILOGUE(r5)

_mem_dspx_write32:
	shlr2	r5
	mov	#0, r0
	lds	r0, x0
	PRELUDE(r5)
1: 2:	movx.l	x0, @r4+
	EPILOGUE(r5)

/* Memory reads and writes from DSP external memory */

.global _mem_dsps_read16
.global _mem_dsps_read32
.global _mem_dsps_write16
.global _mem_dsps_write32

_mem_dsps_read16:
	shlr	r5
	PRELUDE(r5)
1: 2:	movs.w	@r4+, x0
	EPILOGUE(r5)

_mem_dsps_read32:
	shlr2	r5
	PRELUDE(r5)
1: 2:	movs.l	@r4+, x0
	EPILOGUE(r5)

_mem_dsps_write16:
	shlr	r5
	mov	#0, r0
	lds	r0, x0
	PRELUDE(r5)
1: 2:	movs.w	x0, @r4+
	EPILOGUE(r5)

_mem_dsps_write32:
	shlr2	r5
	mov	#0, r0
	lds	r0, x0
	PRELUDE(r5)
1: 2:	movs.l	x0, @r4+
	EPILOGUE(r5)

.align 4
.l4095:
	.long 4095
