/* We put all the code in ILRAM to avoid measurement variations caused by code
   being fetched from ROM. The ILRAM is ideal for this task because successive
   instruction accesses take only 1 cycle (assuming no interference, which
   there is none). */
.section .ilram

/* Test prologue for COUNT iterations (must be a multiple of 256). Note that
   the prologue has an even number of instructions, which results in the loop
   code being 4-aligned, which is of extreme importance. */
#define PROLOGUE(COUNT) 		\
	mov	#(COUNT/256), r0 ;	\
	shll8	r0 ;			\
	ldrs	1f ;			\
	ldre	2f ;			\
	ldrc	r0 ;			\
	nop

/* Test epilogue */
#define EPILOGUE()			\
	rts ;				\
	nop


/* [Baseline]

   In this first section, we find an approximate cost of the setup, which
   consists of TMU access for libprof, function calls, and the loop setup for
   the DSP. This does not include any loop overhead (which is measured later).

   This will often take 3~5 Pϕ/4 ticks, which is not a very precise measure,
   but helps eliminating noise around tests and bringing cycle counts very
   close to multiples of the number of iterations. */

.global _perf_cpu_empty

.align 4
_perf_cpu_empty:
	PROLOGUE(0)
1: 2:	EPILOGUE()


/* [Loop control]

   Here we establish that the DSP repeat system has no added cost per-loop in
   favorable situations. That is, the loop is as efficient as if it were
   unrolled. This is checked by executing the same sequence of instructions
   with a varying number of DSP jumps between them.

   The fact that the DSP jump has no additional cost is very beneficial for
   performance measurements, since it means that variations in the size and
   iteration count of tests has no influence on the results. (Such influence
   would otherwise need to be amortized by unrolling.)

   The only observed difference is with the first test where the single
   instruction in the loop cannot be executed in parallel with itself in the
   next iteration. My guess is that the instruction from the next iteration is
   not fetched yet from the perspective of CPU logic. */

.global _perf_cpu_nop_2048x1

/* nop loop (2048 iterations of 1 nop) -> 2 cycles /i */
.align 4
_perf_cpu_nop_2048x1:
	PROLOGUE(2048)
1: 2: 	nop
	EPILOGUE()

.global _perf_cpu_nop_1024x2

/* nop loop (1024 iterations of 2 nop) -> 1 cycle /i */
.align 4
_perf_cpu_nop_1024x2:
	PROLOGUE(1024)
1:	nop
2:	nop
	EPILOGUE()

.global _perf_cpu_nop_512x4

/* nop loop (512 iterations of 4 nop) -> 2 cycles /i */
.align 4
_perf_cpu_nop_512x4:
	PROLOGUE(512)
1:	nop
	nop
	nop
2:	nop
	EPILOGUE()

.global _perf_cpu_nop_256x8

/* nop loop (256 iterations of 8 nop) -> 4 cycles /i */
.align 4
_perf_cpu_nop_256x8:
	PROLOGUE(256)
1:	nop
	nop
	nop
	nop
	nop
	nop
	nop
2:	nop
	EPILOGUE()

/* [Parallel execution]

   In this section, we reproduce simple cases of superscalar parallelism for
   instructions of different types, using only instructions that have trivial
   pipelines with no extra cycles. */

.global _perf_cpu_EX_EX

/* EX/EX -> 2 cycles /i */
.align 4
_perf_cpu_EX_EX:
	PROLOGUE(1024)
1:	add	#0, r0
2:	add	#0, r1
	EPILOGUE()

.global _perf_cpu_MT_MT

/* MT/MT -> 1 cycle /i */
.align 4
_perf_cpu_MT_MT:
	PROLOGUE(1024)
1:	mov	r0, r1
2:	mov	r2, r3
	EPILOGUE()

.global _perf_cpu_LS_LS

/* LS/LS -> 2 cycles /i */
.align 4
_perf_cpu_LS_LS:
	PROLOGUE(1024)
1:	mov.l	@r15, r0
2:	mov.l	@r15, r1
	EPILOGUE()

/* [Aligned parallelism]

   Here, we show that instruction pairs that are not aligned on 4-byte
   boundaries can nonetheless be parallelized. Having an instruction be
   executed alone because of a lack of parallel-executability with the next one
   does not prevent the next one from forming a parallel pair of its own with
   its successor. */

.global _perf_cpu_align_4

/* 2 pairs of parallel instructions -> 2 cycles /i */
.align 4
_perf_cpu_align_4:
	PROLOGUE(1024)
1:	add	#0, r0
	mov.l	@r15, r1
	add	#0, r0
2:	mov.l	@r15, r1
	EPILOGUE()

.global _perf_cpu_align_2

/* The add/mov.l pair in the middle is parallelized -> 3 cycles /i */
.align 4
_perf_cpu_align_2:
	PROLOGUE(1024)
1:	add	#0, r0
	add	#0, r1
	mov.l	@r15, r0
2:	mov.l	@r15, r1
	EPILOGUE()

/* [Complex pipelines]

   Here we measure the behavior of multi-cycle instructions that have complex
   pipelines. These test establish that while mac.w occupies one pipeline for 2
   cycles, a series of nop can continue to run on the second pipeline.

   Even though mac.w has 2 issue cycles and 4 execution cycles, in a sequence
   of mac.w each instruction will actually take 3 cycles. I believe this is
   because the WB/M2 stage of the second mac.w has a data dependency on the
   MS stage of the previous mac.w instruction, which causes a 1-cycle stall.
   This assumes that there is no forwarding at the output of the multiplier. */

.global _perf_cpu_pipeline_1

/* nop executes in parallel with first pipeline of mac.w -> 3 cycles /i */
.align 4
_perf_cpu_pipeline_1:
	PROLOGUE(1024)
	mov	r15, r0
	mov	r15, r1

1:	mac.w	@r0+, @r1+
2:	nop
	EPILOGUE()

.global _perf_cpu_pipeline_2

/* Without parallel execution, still 3 cycles per mac.w -> 6 cycles /i */
.align 4
_perf_cpu_pipeline_2:
	PROLOGUE(1024)
	mov	r15, r0
	mov	r15, r1

1:	mac.w	@r0+, @r1+
2:	mac.w	@r0+, @r1+
	EPILOGUE()

.global _perf_cpu_pipeline_3

/* mac.w/(nop;nop;nop) then nop/nop -> 4 cycles /i */
.align 4
_perf_cpu_pipeline_3:
	PROLOGUE(1024)
	mov	r15, r0
	mov	r15, r1

1:	mac.w	@r0+, @r1+
	nop
	nop
	nop
	nop
2:	nop
	EPILOGUE()

/* [RAW dependencies]

   In this section we establish the delay caused by RAW dependencies in
   arithmetic and memory access instructions. */

.global _perf_cpu_raw_EX_EX

.align 4
_perf_cpu_raw_EX_EX:
	PROLOGUE(1024)
1:	add	#1, r0
2:	add	#1, r0
	EPILOGUE()

.global _perf_cpu_raw_LS_LS

.align 4
_perf_cpu_raw_LS_LS:
	PROLOGUE(1024)
	mov.l	.buffer, r4
	nop

1:	mov.l	@r4, r0
2:	mov.l	r0, @r4
	EPILOGUE()

.global _perf_cpu_raw_EX_LS

.align 4
_perf_cpu_raw_EX_LS:
	PROLOGUE(1024)
	mov.l	.buffer, r4
	mov	#0, r0

1:	add	#1, r0
2:	mov.l	r0, @r4
	EPILOGUE()

.global _perf_cpu_raw_LS_EX

.align 4
_perf_cpu_raw_LS_EX:
	PROLOGUE(1024)
	mov.l	.buffer, r4
	nop

1:	mov.l	@r4, r0
2:	add	#1, r0
	EPILOGUE()

.global _perf_cpu_noraw_LS_LS

.align 4
_perf_cpu_noraw_LS_LS:
	PROLOGUE(1024)
	mov.l	.buffer, r4
	nop

1:	mov.l	@r4, r0
2:	mov.l	r1, @r4
	EPILOGUE()

.global _perf_cpu_noraw_LS_EX

.align 4
_perf_cpu_noraw_LS_EX:
	PROLOGUE(1024)
	mov.l	.buffer, r4
	nop

1:	mov.l	@r4, r0
2:	add	#1, r1
	EPILOGUE()

.global _perf_cpu_raw_EX_LS_addr

.align 4
_perf_cpu_raw_EX_LS_addr:
	PROLOGUE(1024)
	mov.l	.buffer, r4
	nop

1:	add	#0, r4
2:	mov.l	r0, @r4
	EPILOGUE()

.global _perf_cpu_raw_DSPLS_DSPLS

.align 4
_perf_cpu_raw_DSPLS_DSPLS:
	PROLOGUE(512)
	mov.l	.buffer, r4
	mov	r4, r5

1:	movs.w	@r4, x0
2:	movs.w	x0, @r5
	EPILOGUE()

/* [Iteration weaving]

   In this section we analyze how iterations can be woven and opened to improve
   performance by reducing RAW dependencies. */

.global _perf_cpu_darken_1

.align 4
_perf_cpu_darken_1:
	PROLOGUE(512)
	mov.l	.buffer, r4
	mov	r4, r5
	add	#-4, r5
	nop

1:	mov.l	@r4+, r1
	and	r2, r1
	add	#4, r5
	shlr	r1
2:	mov.l	r1, @r5
	EPILOGUE()

.global _perf_cpu_darken_2

.align 4
_perf_cpu_darken_2:
	PROLOGUE(512)
	mov.l	.buffer, r4
	mov	r4, r5
	add	#-4, r5
	nop

1:	mov.l	@r4+, r1
	add	#4, r5
	and	r2, r1
	shlr	r1
2:	mov.l	r1, @r5
	EPILOGUE()

.global _perf_cpu_darken_3

.align 4
_perf_cpu_darken_3:
	PROLOGUE(256)
	mov.l	.buffer, r4
	mov	r4, r5
	add	#-8, r5
	nop

1:	mov.l	@r4+, r1
	add	#8, r5
	mov.l	@r4+, r3
	and	r2, r1
	shlr	r1
	mov.l	r1, @r5
	and	r2, r3
	shlr	r3
2:	mov.l	r3, @(4,r5)
	EPILOGUE()

.global _perf_cpu_darken_4

.align 4
_perf_cpu_darken_4:
	PROLOGUE(256)
	mov.l	.buffer, r4
	mov	r4, r5
	add	#-8, r5
	mov.l	@r4+, r1

	/* Loop starts with r1 loaded, finishes with r1 loaded */
1:	mov.l	@r4+, r3
	add	#8, r5
	and	r2, r1
	shlr	r1
	mov.l	r1, @r5
	mov.l	@r4+, r1
	and	r2, r3
	shlr	r3
2:	mov.l	r3, @(4,r5)
	EPILOGUE()

/* [Advanced dependencies]

   This section measures the delay needed to use registers depending on the
   type of instruction which modifies them. */

.global _perf_cpu_double_read

.align 4
_perf_cpu_double_read:
	PROLOGUE(1024)
	mov.l	.buffer, r4
	nop

1:	mov.l	@r4, r0
2:	mov.l	@r4, r1
	EPILOGUE()

.global _perf_cpu_double_incr_read

.align 4
_perf_cpu_double_incr_read:
	PROLOGUE(1024)
	mov.l	.buffer, r4
	nop

1:	mov.b	@r4+, r0
2:	mov.b	@r4+, r0
	EPILOGUE()

/* [2D texture copy]

   This section is used to investigate the performance of the 2D texture shader
   of azur. */

.global _perf_cpu_tex2d

.align 4
_perf_cpu_tex2d:
	PROLOGUE(512)
	mov.l	.buffer2, r3
	mov	r3, r5 /*.buffer, r5 */

1:	movs.l	@r3+, x0
2:	movs.l	x0, @r5+
	EPILOGUE()

/* XRAM buffer */

.align 4
.buffer:
	.long	_cpu_perf_xram_buffer
.buffer2:
	.long	_buffer2

.section .data
_buffer2:
	.zero	2048
