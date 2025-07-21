#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/clock.h>

#include <gintctl/perf.h>
#include <gintctl/util.h>
#include <gintctl/plot.h>

#include <gint/prof.h>

#include <stdio.h>
#include <stdlib.h>

#define CACHE_MAX 65536
#define SAMPLES 129

extern void cpucache_nop1024(int repeats);
extern void cpucache_rounds(uint8_t const *buf, size_t len, int rounds);

uint32_t test_nop4096(void)
{
	prof_t prof = prof_make();

	prof_enter(prof);
	cpucache_nop1024(4);
	prof_leave(prof);

	/* Return the amount in Iphi cycles computed from the Pphi/4 measure */
	clock_frequency_t const *freq = clock_freq();
	uint64_t PLL_cycles = ((uint64_t)prof.elapsed * 4) * freq->Pphi_div;
	return PLL_cycles / freq->Iphi_div;
}

uint32_t test_cpucache_rounds(uint8_t const *buf, size_t len, int rounds)
{
	prof_t prof = prof_make();

	prof_enter(prof);
	cpucache_rounds(buf, len, rounds);
	prof_leave(prof);

	clock_frequency_t const *freq = clock_freq();
	uint64_t PLL_cycles = ((uint64_t)prof.elapsed * 4) * freq->Pphi_div;
	return PLL_cycles / freq->Iphi_div;
}

#if GINT_RENDER_MONO
static void tick_formatter(char *str, size_t size, int32_t v)
{
	if(v == 0) snprintf(str, size, "0");
	else snprintf(str, size, "%dk", v/1000);
}
#endif

/* gintctl_perf_cpucache(): CPU speed and cache size */
void gintctl_perf_cpucache(void)
{
	int key = 0;

	/* Test twice because this is sensitive to libprof initialization */
	uint32_t nop4096 = test_nop4096();
	nop4096 = test_nop4096();

	uint8_t *buf = malloc(CACHE_MAX);
	if(!buf) return;

	int32_t x_size[SAMPLES];
	int32_t y_time[SAMPLES];

	struct plot plotspec = {
		.data_x = x_size,
		.data_y = y_time,
		.data_len = SAMPLES,

		#if GINT_RENDER_MONO
		.area = {
			.x = 0, .y = 18,
			.w = 128, .h = 44,
		},
		.color = C_BLACK,
		.grid = {
			.level = PLOT_MAINGRID,
			.primary_color = C_BLACK,
			.dotted = 1,
		},
		#endif

		#if GINT_RENDER_RGB
		.area = {
			.x = 24, .y = 51,
			.w = 340, .h = 120,
		},
		.color = C_RED,
		.grid = {
			.level = PLOT_FULLGRID,
			.primary_color = C_RGB(20, 20, 20),
			.secondary_color = C_RGB(28, 28, 28),
			.dotted = 1,
		},
		#endif

		#if GINT_HW_FX
		.ticks_x = {
			.multiples = 250,
			.subtick_divisions = 4,
			.formatter = tick_formatter,
		},
		.ticks_y = {
			.multiples = 20000,
			.subtick_divisions = 2,
			.formatter = tick_formatter,
		},
		#endif

		#if GINT_HW_CG
		.ticks_x = {
			.multiples = CACHE_MAX / 16,
			.subtick_divisions = 4,
			#if GINT_RENDER_MONO
			.formatter = tick_formatter,
			#endif
		},
		.ticks_y = {
			.multiples = 125000,
			.subtick_divisions = 2,
			#if GINT_RENDER_MONO
			.formatter = tick_formatter,
			#endif
		},
		#endif
	};

	int y_min = -1;
	int y_max = -1;

	for(int i = 0; i < SAMPLES; i++)
	{
		x_size[i] = (CACHE_MAX / (SAMPLES-1)) * i;
		y_time[i] = test_cpucache_rounds(buf, x_size[i], 8);

		if(y_time[i] < y_min || y_min == -1) y_min = y_time[i];
		if(y_time[i] > y_max || y_max == -1) y_max = y_time[i];
	}

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);
		row_title(_("CPU and cache", "CPU speed and cache size"));

		#if GINT_RENDER_MONO
		row_print(2, 1, "4096 nop: %d Iϕ", nop4096);
		extern font_t font_hexa;
		font_t const *old = dfont(&font_hexa);
		plot(&plotspec);
		dfont(old);
		#endif

		#if GINT_RENDER_RGB
		row_print(1, 1, "Time for 4096 nop (with overhead): %d Iphi",
			nop4096);
		row_print(2, 1, "Time needed to read a buffer multiple times:");

		plot(&plotspec);

		row_print(12, 1, "X: Size of buffer (bytes)");
		row_print(13, 1, "Y: Iphi cycles for 8x 32-bit traversals");
		row_print(14, 1, "Last samples suggests: %.2D Iphi/byte access",
			100 * y_time[SAMPLES-1] / x_size[SAMPLES-1] / 8);
		#endif

		dupdate();
		key = getkey().key;
	}
}
