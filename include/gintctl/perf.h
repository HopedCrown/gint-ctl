//---
//	gintctl:perf - Performance and benchmarks
//---

#ifndef GINTCTL_PERF
#define GINTCTL_PERF

/* gintctl_perf_libprof(): Basic libprof tests using timers */
void gintctl_perf_libprof(void);

/* gintctl_perf_cpucache(): CPU speed and cache size */
void gintctl_perf_cpucache(void);

/* gintctl_perf_cpu(): CPU instruction parallelism and pipelining */
void gintctl_perf_cpu(void);

/* gintctl_perf_interrupts(): Interrupt handling */
void gintctl_perf_interrupts(void);

/* gintctl_perf_memory(): Memory primitives and reading/writing speed */
void gintctl_perf_memory(void);

/* gintctl_perf_render(): Profile the display primitives */
void gintctl_perf_render(void);

#endif /* GINTCTL_PERF */
