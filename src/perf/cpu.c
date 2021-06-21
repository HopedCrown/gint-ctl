#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/clock.h>

#include <gintctl/perf.h>
#include <gintctl/util.h>

#include <libprof.h>

/* Baseline */
void perf_cpu_empty(void);
/* Loop control */
void perf_cpu_nop_2048x1(void);
void perf_cpu_nop_1024x2(void);
void perf_cpu_nop_512x4(void);
void perf_cpu_nop_256x8(void);
/* Parallel execution */
void perf_cpu_EX_EX(void);
void perf_cpu_MT_MT(void);
void perf_cpu_LS_LS(void);

/* Is subtracted from result times if specified; in TMU units (prof.elapsed) */
static uint32_t baseline_ticks = 0;

/* Number of CPU cycles spent executing a function */
uint32_t Iphi_cycles(void (*function)(void))
{
	prof_t perf = prof_make();

	prof_enter(perf);
	(*function)();
	prof_leave(perf);

	clock_frequency_t const *freq = clock_freq();
	uint32_t TMU_cycles = perf.elapsed - baseline_ticks;
	uint32_t PLL_cycles = (TMU_cycles * 4) * freq->Pphi_div;
	return PLL_cycles / freq->Iphi_div;
}

/* Number of CPU cycles per iteration; the number of iterations must obviously
   match assembler code for that test */
float Iphi_per_iteration(void (*function)(void), int count)
{
	return (float)Iphi_cycles(function) / count;
}

/* Number of TMU cycles for an empty function */
uint32_t TMU_baseline(void)
{
	prof_t perf = prof_make();

	for(int i = 0; i < 16; i++)
	{
		prof_enter(perf);
		perf_cpu_empty();
		prof_leave(perf);
	}

	return perf.elapsed / 16;
}

//---

void gintctl_perf_cpu(void)
{
	int key = 0;

	/* Measure baseline time */
	baseline_ticks = TMU_baseline();

	uint32_t Iphi_cpu_nop_2048x1 = 0;
	uint32_t Iphi_cpu_nop_1024x2 = 0;
	uint32_t Iphi_cpu_nop_512x4 = 0;
	uint32_t Iphi_cpu_nop_256x8 = 0;

	uint32_t Iphi_cpu_EX_EX = 0;
	uint32_t Iphi_cpu_MT_MT = 0;
	uint32_t Iphi_cpu_LS_LS = 0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FXCG50
		row_title("CPU instruction parallelism and pipelining");

		row_print(1, 1, "Baseline ticks: %d",
			baseline_ticks);
		row_print(3, 1, "Iphi cycles for 2048x1 nop: %d",
			Iphi_cpu_nop_2048x1);
		row_print(4, 1, "Iphi cycles for 1024x2 nop: %d",
			Iphi_cpu_nop_1024x2);
		row_print(5, 1, "Iphi cycles for 512x4 nop: %d",
			Iphi_cpu_nop_512x4);
		row_print(6, 1, "Iphi cycles for 256x8 nop: %d",
			Iphi_cpu_nop_256x8);
		row_print(8, 1, "Iphi cycles for EX/EX: %d",
			Iphi_cpu_EX_EX);
		row_print(9, 1, "Iphi cycles for MT/MT: %d",
			Iphi_cpu_MT_MT);
		row_print(10, 1, "Iphi cycles for LS/LS: %d",
			Iphi_cpu_LS_LS);

		fkey_button(1, "RUN");
		#endif

		dupdate();
		key = getkey().key;

		if(key == KEY_F1)
		{
			Iphi_cpu_nop_2048x1 = Iphi_cycles(perf_cpu_nop_2048x1);
			Iphi_cpu_nop_1024x2 = Iphi_cycles(perf_cpu_nop_1024x2);
			Iphi_cpu_nop_512x4  = Iphi_cycles(perf_cpu_nop_512x4);
			Iphi_cpu_nop_256x8  = Iphi_cycles(perf_cpu_nop_256x8);

			Iphi_cpu_EX_EX = Iphi_cycles(perf_cpu_EX_EX);
			Iphi_cpu_MT_MT = Iphi_cycles(perf_cpu_MT_MT);
			Iphi_cpu_LS_LS = Iphi_cycles(perf_cpu_LS_LS);
		}
	}
}
