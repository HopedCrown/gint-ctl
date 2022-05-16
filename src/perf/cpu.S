/* We put all the code in ILRAM to avoid measurement variations caused by code
   being fetched from ROM. The ILRAM is ideal for this task because successive
   instruction accesses take only 1 cycle (assuming no interference, which
   there is none). */
.section .ilram, "ax"

/* Test prologue for COUNT iterations (must be a multiple of 256). Note that
   the prologue has an even number of instructions, which results in the loop
   code being 4-aligned, which is of extreme importance. */
.macro bench SYMBOL, COUNT
.global _perf_cpu_\SYMBOL
.align 4
_perf_cpu_\SYMBOL:
	mov	#(\COUNT/256), r0
	shll8	r0
	ldrs	1f
	ldre	2f
	ldrc	r0
	nop
.endm

/* Epilogue */
.macro end
	rts
	nop
.endm

/* Same for CPU loops, which are sometimes relevant */
.macro benchcpu SYMBOL, COUNT
.global _perf_cpu_\SYMBOL
.align 4
_perf_cpu_\SYMBOL:
	mov	#(\COUNT/256), r0
	shll8	r0
.endm

.macro endcpu
	dt	r0
	nop

	bf.s	1b
	nop

	rts
	nop
.endm

/* [Baseline]

   In this first section, we find an approximate cost of the setup, which
   consists of TMU access for libprof, function calls, and the loop setup for
   the DSP. This does not include any loop overhead (which is measured later).

   This will often take 3~5 Pϕ/4 ticks, which is not a very precise measure,
   but helps eliminating noise around tests and bringing cycle counts very
   close to multiples of the number of iterations. */

bench empty, 0
1: 2:
end

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

/* nop loop (2048 iterations of 1 nop) -> 2 cycles /i */
bench nop_2048x1, 2048
1: 2: 	nop
end

/* nop loop (1024 iterations of 2 nop) -> 1 cycle /i */
bench nop_1024x2, 1024
1:	nop
2:	nop
end

/* nop loop (512 iterations of 4 nop) -> 2 cycles /i */
bench nop_512x4, 512
1:	nop
	nop
	nop
2:	nop
end

/* nop loop (256 iterations of 8 nop) -> 4 cycles /i */
bench nop_256x8, 256
1:	nop
	nop
	nop
	nop
	nop
	nop
	nop
2:	nop
end

/* nop loop (1024 iterations of 2 nop) + CPU loop -> ??? cycles /i */
benchcpu nop_1024x2_cpuloop, 1024
1:	nop
	nop
endcpu

/* nop loop (512 iterations of 4 nop) + CPU loop -> ??? cycles /i */
benchcpu nop_512x4_cpuloop, 512
1:	nop
	nop
	nop
	nop
endcpu

/* nop loop (256 iterations of 8 nop) + CPU loop -> ??? cycles /i */
benchcpu nop_256x8_cpuloop, 256
1:	nop
	nop
	nop
	nop
	nop
	nop
	nop
2:	nop
endcpu

/* [Parallel execution]

   In this section, we reproduce simple cases of superscalar parallelism for
   instructions of different types, using only instructions that have trivial
   pipelines with no extra cycles. */

/* EX/EX -> 2 cycles /i */
bench EX_EX, 1024
1:	add	#0, r0
2:	add	#0, r1
end

/* MT/MT -> 1 cycle /i */
bench MT_MT, 1024
1:	mov	r0, r1
2:	mov	r2, r3
end

/* LS/LS -> 2 cycles /i */
bench LS_LS, 1024
1:	mov.l	@r15, r0
2:	mov.l	@r15, r1
end

/* [Aligned parallelism]

   Here, we show that instruction pairs that are not aligned on 4-byte
   boundaries can nonetheless be parallelized. Having an instruction be
   executed alone because of a lack of parallel-executability with the next one
   does not prevent the next one from forming a parallel pair of its own with
   its successor. */

/* 2 pairs of parallel instructions -> 2 cycles /i */
bench align_4, 1024
1:	add	#0, r0
	mov.l	@r15, r1
	add	#0, r0
2:	mov.l	@r15, r1
end

/* The add/mov.l pair in the middle is parallelized -> 3 cycles /i */
bench align_2, 1024
1:	add	#0, r0
	add	#0, r1
	mov.l	@r15, r0
2:	mov.l	@r15, r1
end

/* [Complex pipelines]

   Here we measure the behavior of multi-cycle instructions that have complex
   pipelines. These test establish that while mac.w occupies one pipeline for 2
   cycles, a series of nop can continue to run on the second pipeline.

   Even though mac.w has 2 issue cycles and 4 execution cycles, in a sequence
   of mac.w each instruction will actually take 3 cycles. I believe this is
   because the WB/M2 stage of the second mac.w has a data dependency on the
   MS stage of the previous mac.w instruction, which causes a 1-cycle stall.
   This assumes that there is no forwarding at the output of the multiplier. */

/* nop executes in parallel with first pipeline of mac.w -> 3 cycles /i */
bench pipeline_1, 1024
	mov	r15, r0
	mov	r15, r1

1:	mac.w	@r0+, @r1+
2:	nop
end

/* Without parallel execution, still 3 cycles per mac.w -> 6 cycles /i */
bench pipeline_2, 1024
	mov	r15, r0
	mov	r15, r1

1:	mac.w	@r0+, @r1+
2:	mac.w	@r0+, @r1+
end

/* mac.w/(nop;nop;nop) then nop/nop -> 4 cycles /i */
bench pipeline_3, 1024
	mov	r15, r0
	mov	r15, r1

1:	mac.w	@r0+, @r1+
	nop
	nop
	nop
	nop
2:	nop
end

/* [RAW dependencies]

   In this section we establish the delay caused by RAW dependencies in
   arithmetic and memory access instructions. */

/* Forwarding after the ALU is seamless, no delay -> 2 cycles /i */
bench raw_EX_EX, 1024
1:	add	#1, r0
2:	add	#1, r0
end

/* Value is available immediately for memory... at a *different address* (the
   same addresse would give 4 cycles /i) -> 2 cycles /i */
bench raw_LS_LS, 1024
	mov.l	.buffer, r4
	nop

1:	mov.l	@r4, r0
2:	mov.l	r0, @(4,r4)
end

/* Perfect forwarding from ALU to memory access -> 1 cycle /i */
bench raw_EX_LS, 1024
	mov.l	.buffer, r4
	mov	#0, r0

1:	add	#1, r0
2:	mov.l	r0, @r4
end

/* 1-cycle stall after loading a register from memory -> 3 cycles /i */
bench raw_LS_EX, 1024
1:	mov.l	@r15, r0
2:	add	#1, r0
end

/* Same - it's not like you could move to avoid the stall -> 3 cycles /i */
bench raw_LS_MT, 1024
1:	mov.l	@r15, r0
2:	mov	r0, r1
end

/* Efficient as expected -> 1 cycle /i */
bench raw_EX_MT, 2048
1:	add	#0, r4
2:	mov	r4, r5
end

/* Efficient as expected -> 1 cycle /i */
bench raw_MT_EX, 2048
1:	mov	r5, r4
2:	add	#0, r4
end

/* The addresses must be different -> 2 cycles /i */
bench raw_DSPLS_DSPLS, 512
	mov.l	.buffer, r4
	mov	r4, r5
	add	#2, r5
	nop

1:	movs.w	@r4, x0
2:	movs.w	x0, @r5
end

/* Still efficient as long as the addresses are different -> 2 cycles /i */
bench noraw_LS_LS, 1024
	mov.l	.buffer, r4
	mov	r4, r5

1:	mov.l	@r4, r0
2:	mov.l	r1, @(4,r5)
end

/* Normal superscalar parallelism at work -> 1 cycle /i */
bench noraw_LS_EX, 1024
	mov.l	.buffer, r4
	nop

1:	mov.l	@r4, r0
2:	add	#1, r1
end

/* Works no problem, MT is very friendly -> 1 cycle /i  */
bench raw_MT_LS_addr, 1024
	mov.l	.buffer, r5
	nop

1:	mov	r5, r4
2:	mov.l	r0, @r4
end

/* There is no forwarding on the address, so similar to loading this actually
   takes much longer than when modifying the operand -> 3 cycles /i */
bench raw_EX_LS_addr, 1024
	mov.l	.buffer, r4
	nop

1:	add	#0, r4
2:	mov.l	r0, @r4
end

/* Same process for the index -> 3 cycles /i
   Also more results (maybe loads into r0 are delayed by the next iteration?)
     EX on r0/LS indexing r0, into rm (m != 0)   -> 3 cycles /i
     EX in r0/LS indexing r0, into r0            -> 4 cycles /i (!)
     MT in r0/LS indexing r0, into rm (m != 0)   -> 1 cycle  /i
     MT in r0/LS indexing r0, into r0            -> 1 cycle  /i */
bench raw_EX_LS_index, 1024
	mov.l	.buffer, r4
	mov	#0, r6

1:	mov	r6, r0
2:	mov.l	@(r0,r4), r0
end

/* The worst of all; 2-cycle stall to use a loaded address -> 4 cycles /i */
bench raw_LS_LS_addr, 1024
	mov.l	.buffer, r4
	mov.l	r15, @r4

1:	mov.l	@r4, r5
2:	mov.l	@r5, r6
end

/* [Branching]

   In this section, we investigate the cost of conditional execution and
   branching, which features both delay slots and pipeline bubbles. */

/* WOW. 18 cycles /i in ILRAM, 12 cycles /i in ROM and RAM. What the heck is
   instruction prefetching doing with this? */
bench branch_bra, 1024
1:	bra	3f
	nop
     3: nop
2:	nop
end

.global _perf_cpu_branch_bra_cpuloop
.align 4
_perf_cpu_branch_bra_cpuloop:
	mov	#4, r0
	shll8	r0
1:	bra	3f
	dt	r0
     3: bf.s	1b
	nop
	rts
	nop

/* [Loop unrolling]

   In this section we analyze how loops can be unrolled and pipeliend to
   improve performance by reducing RAW dependencies. This is illustrated with a
   function that darkens a continuous section of VRAM. The initial version
   takes 3 cycles /pixel, whereas the optimized takes 1.25 cycle /pixel. */

/* Darkening RGB565 by (color = (color & 0xf7de) >> 1). This base version does
   two pixels at a time but has pretty complex RAWs -> 6 cycles /i */
bench darken_1, 512
	mov.l	.buffer, r4
	mov	r4, r5
	add	#-4, r5
	nop

1:	mov.l	@r4+, r1
	/* Stall because of loading r1 */
	and	r2, r1
	add	#4, r5
	shlr	r1
	/* Stall because of access to r5 as address */
2:	mov.l	r1, @r5
end

/* Here the change to r5 is moved to eliminate both stalls -> 4 cycles /i */
bench darken_2, 512
	mov.l	.buffer, r4
	mov	r4, r5
	add	#-4, r5
	nop

1:	mov.l	@r4+, r1
	add	#4, r5
	and	r2, r1
	/* The EX/LS pair below is the only one parallelized */
	shlr	r1
2:	mov.l	r1, @r5
end

/* Here iterations are unrolled to increase the amount of independent data.
   Each iteration processes twice as much data and uses EX cycles of the first
   longword to do LS work on the second one. Plus r5 is incremented only once
   -> 6 cycles /i */
bench darken_3, 256
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
end

/* Finally iterations are pipelined here to eliminate the long chain of
   dependency from the loads to the stores. Late EX instructions are
   parallelized with loads for the next iteration -> 5 cycles/i */
bench darken_4, 256
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
end

/* [Advanced dependencies]

   This section measures the delay needed to use registers depending on the
   type of instruction which modifies them. */

/* No problem here -> 2 cycles /i */
bench double_read, 1024
	mov.l	.buffer, r4
	nop

1:	mov.l	@r4, r0
2:	mov.l	@r4, r1
end

/* Post-increment feeds into address much faster than ALU -> 2 cycles /i */
bench double_incr_read, 1024
	mov.l	.buffer, r4
	nop

1:	mov.b	@r4+, r0
2:	mov.b	@r4+, r0
end

/* No delay writing twice, whether with r4/r4 or r4/r5 -> 2 cycles/i */
bench double_write, 1024
	mov.l	.buffer, r4
	mov.l	@r4, r0
	mov	r0, r1
	mov	r4, r5

1:	mov.l	r0, @r4
2:	mov.l	r1, @r5
end

/* XRAM buffer */

.align 4
.buffer:
	.long	_cpu_perf_xram_buffer
