#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/clock.h>

#include <gintctl/perf.h>
#include <gintctl/util.h>
#include <gintctl/assets.h>

#include <gintctl/widgets/gscreen.h>
#include <gintctl/widgets/gtable.h>

#include <libprof.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GXRAM uint32_t cpu_perf_xram_buffer[512];

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
int Iphi_cycles_per_iteration(int total, int count)
{
	div_t d = div(total, count);

	if(d.rem < 192)
		return d.quot;
	if(d.rem > count - 192)
		return d.quot + 1;

	return -1;
}

/* Number of TMU cycles for an empty function */
uint32_t TMU_baseline(void)
{
	void perf_cpu_empty(void);
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

struct results {
	int nop_2048x1, nop_1024x2, nop_512x4, nop_256x8;
	int EX_EX, MT_MT, LS_LS;
	int align_4, align_2;
	int pipeline_1, pipeline_2, pipeline_3;
	int raw_EX_EX, raw_LS_LS, raw_EX_LS, raw_LS_EX, raw_LS_MT;
	int noraw_LS_LS, noraw_LS_EX;
	int raw_EX_LS_addr, raw_EX_LS_index, raw_LS_LS_addr, raw_DSPLS_DSPLS;
	int darken_1, darken_2, darken_3, darken_4;
	int double_read, double_incr_read, double_write;
	#ifdef FXCG50
	int tex2d;
	#endif
};

/* Number of Iphi cycles total, and number of iterations */
static struct results r_cycles, r_iter;

static void table_gen(gtable *t, int row)
{
	static char const *names[] = {
		"Single nop", "2 nop", "4 nop", "8 nop",
		"EX/EX pair", "MT/MT pair", "LS/LS pair",
		"4-aligned parallel pair", "2-aligned parallel pair",
		"mac.w/nop pipeline", "mac.w/mac.w pipeline",
		  "mac.w/nop*5 pipeline",
		"RAW dep.: EX/EX", "RAW dep.: LS/LS", "RAW dep.: EX/LS",
		  "RAW dep.: LS/EX", "RAW dep.: LS/MT",
		  "No dep.: LS/LS", "No dep.: LS/EX",
		  "RAW on address: EX/LS", "RAW on index: EX/LS",
		  "RAW on address: LS/LS",
		  "RAW dep.: DSP-LS/DSP-LS",
		"32-bit VRAM darken #1", "32-bit VRAM darken #2",
		  "Interwoven darken", "Interwoven open darken",
		"Double read", "Double increment read", "Double write",
		"Texture2D shader",
	};

	int cycles = ((int *)&r_cycles)[row];
	int iter = ((int *)&r_iter)[row];
	int cpi = Iphi_cycles_per_iteration(cycles, iter);

	char c2[16], c3[16], c4[16];
	sprintf(c2, "%d", cpi);
	sprintf(c3, "%d", cycles);
	sprintf(c4, "%d", iter);

	gtable_provide(t, names[row], (cpi == -1 ? "-" : c2), c3, c4);
}

void gintctl_perf_cpu(void)
{
	memset(&r_cycles, 0, sizeof r_cycles);
	memset(&r_iter, 0, sizeof r_iter);

	gtable *table = gtable_create(4, table_gen, NULL, NULL);
	gtable_set_rows(table, sizeof r_cycles / sizeof(int));
	gtable_set_row_spacing(table, _(1,2));
	gtable_set_column_titles(table, "Name", "CPI", "Cycles", "Iter.");
	gtable_set_column_sizes(table, 6, 1, 2, 2);
	gtable_set_font(table, _(&font_mini, dfont_default()));
	jwidget_set_margin(table, 0, 2, 1, 2);

	gscreen *scr = gscreen_create2("CPU parallelism", &img_opt_perf_cpu,
		"CPU instruction parallelism and pipelining", "@RUN;;;;;");
	gscreen_add_tabs(scr, table, table);
	jscene_set_focused_widget(scr->scene, table);

	int key = 0;
	while(key != KEY_EXIT) {
		jevent e = jscene_run(scr->scene);

		if(e.type == JSCENE_PAINT) {
			dclear(C_WHITE);
			jscene_render(scr->scene);
			dupdate();
		}

		key = 0;
		if(e.type == JSCENE_KEY && e.key.type == KEYEV_DOWN)
			key = e.key.key;

		if(key == KEY_F1) {
			baseline_ticks = TMU_baseline();

			#define run(name, iter) {								\
				extern void perf_cpu_ ## name (void);				\
				r_cycles.name = Iphi_cycles(perf_cpu_ ## name);		\
				r_iter.name = iter;									\
			}

			run(nop_2048x1, 2048);
			run(nop_1024x2, 1024);
			run(nop_512x4, 512);
			run(nop_256x8, 256);

			run(EX_EX, 1024);
			run(MT_MT, 1024);
			run(LS_LS, 1024);

			run(align_4, 1024);
			run(align_2, 1024);

			run(pipeline_1, 1024);
			run(pipeline_2, 1024);
			run(pipeline_3, 1024);

			run(raw_EX_EX, 1024);
			run(raw_LS_LS, 1024);
			run(raw_EX_LS, 1024);
			run(raw_LS_EX, 1024);
			run(raw_LS_MT, 1024);
			run(noraw_LS_LS, 1024);
			run(noraw_LS_EX, 1024);
			run(raw_EX_LS_addr, 1024);
			run(raw_EX_LS_index, 1024);
			run(raw_LS_LS_addr, 1024);
			run(raw_DSPLS_DSPLS, 512);

			run(darken_1, 512);
			run(darken_2, 512);
			run(darken_3, 256);
			run(darken_4, 256);

			run(double_read, 1024);
			run(double_incr_read, 1024);
			run(double_write, 1024);

			#ifdef FXCG50
			run(tex2d, 512);
			#endif

			table->widget.update = 1;
		}
	}

	gscreen_destroy(scr);
}
