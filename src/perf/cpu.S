# We put all the code in ILRAM to avoid measurement variations caused by code
# being fetched from ROM. The ILRAM is ideal for this task because successive
# instruction accesses take only 1 cycle (assuming no interference, which there
# is none).
.section .ilram

# Test prologue for COUNT iterations (must be a multiple of 256). Note that the
# prologue has an even number of instructions, which results in the loop code
# being 4-aligned, which is of extreme importance.
#define PROLOGUE(COUNT) 		\
	mov	#(COUNT/256), r0 ;	\
	shll8	r0 ;			\
	ldrs	1f ;			\
	ldre	2f ;			\
	ldrc	r0 ;			\
	nop

# Test epilogue
#define EPILOGUE()			\
	rts ;				\
	nop


/* [Baseline]

   In this first section, we want to establish an approximate cost of the
   setup, which consists of TMU access for libprof, function calls, and the
   loop setup for the DSP. */

.global _perf_cpu_empty

# Empty setup (0 iterations), as baseline
.align 4
_perf_cpu_empty:
	PROLOGUE(0)
1: 2:	EPILOGUE()


/* [Loop control]

   In this section, we want to check whether the DSP repeat system has any
   added cost per-loop, and we do this by executing the same instructions with
   a varying number of DSP repeat jumps.

   The DSP jump has no additional cost, which makes testing much simpler by
   avoiding loop unrolls (that would otherwise be needed to amortize the cost
   of the jump). In addition, this allows for tighter loops in real-world
   programs, and tigher code first better in cache. */

.global _perf_cpu_nop_2048x1
.global _perf_cpu_nop_1024x2
.global _perf_cpu_nop_512x4
.global _perf_cpu_nop_256x8

# nop loop (2048 iterations of 1 nop) -> 2 cycles /i
# Parallel execution likely cannot read through the DSP jump for architectural
# reasons; my guess is that the next instruction isn't fetched yet.
.align 4
_perf_cpu_nop_2048x1:
	PROLOGUE(2048)
1: 2: 	nop
	EPILOGUE()

# nop loop (1024 iterations of 2 nop) -> 1 cycle /i
.align 4
_perf_cpu_nop_1024x2:
	PROLOGUE(1024)
1:	nop
2:	nop
	EPILOGUE()

# nop loop (512 iterations of 4 nop) -> 2 cycles /i
.align 4
_perf_cpu_nop_512x4:
	PROLOGUE(512)
1:	nop
	nop
	nop
2:	nop
	EPILOGUE()

# nop loop (256 iterations of 8 nop) -> 4 cycles/i
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
.global _perf_cpu_MT_MT
.global _perf_cpu_LS_LS

# EX/EX (1024 iterations of 2 non-parallel instructions) -> 2 cycles /i
.align 4
_perf_cpu_EX_EX:
	PROLOGUE(1024)
1:	add	#0, r0
2:	add	#0, r1
	EPILOGUE()

# MT/MT (1024 iterations of 2 parallel instructions) -> 1 cycle /i
.align 4
_perf_cpu_MT_MT:
	PROLOGUE(1024)
1:	mov	r0, r1
2:	mov	r2, r3
	EPILOGUE()

# LS/LS (1024 iterations of 2 non-parallel instructions) -> 2 cycles /i
.align 4
_perf_cpu_LS_LS:
	PROLOGUE(1024)
1:	mov.l	@r15, r0
2:	mov.l	@r15, r1
	EPILOGUE()
