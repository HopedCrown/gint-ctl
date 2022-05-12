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

/* List of all tests with the macro expansion trick */
#define ALL_TESTS(MACRO) \
	MACRO(nop_2048x1,			2048,	"Single nop") \
	MACRO(nop_1024x2,			1024,	"2 nop") \
	MACRO(nop_512x4,			512,	"4 nop") \
	MACRO(nop_256x8,			256,	"8 nop") \
	MACRO(nop_1024x2_cpuloop,	1024,	"2 nop (CPU loop)") \
	MACRO(nop_512x4_cpuloop,	512,	"4 nop (CPU loop)") \
	MACRO(nop_256x8_cpuloop,	256,	"8 nop (CPU loop)") \
	MACRO(EX_EX,				1024,	"Normal pair: EX/EX") \
	MACRO(MT_MT,				1024,	"Normal pair: MT/MT") \
	MACRO(LS_LS,				1024,	"Normal pair: LS/LS") \
	MACRO(align_4,				1024,	"Normal-pair: 4-aligned") \
	MACRO(align_2,				1024,	"Normal pair: 2-aligned") \
	MACRO(pipeline_1,			1024,	"Pipeline: mac.w/nop") \
	MACRO(pipeline_2,			1024,	"Pipeline: mac.w/mac.w") \
	MACRO(pipeline_3,			1024,	"Pipeline: mac.w/nop*5") \
	MACRO(raw_EX_EX,			1024,	"RAW on data: EX/EX") \
	MACRO(raw_LS_LS,			1024,	"RAW on data: LS/LS") \
	MACRO(raw_EX_LS,			1024,	"RAW on data: EX/LS") \
	MACRO(raw_LS_EX,			1024,	"RAW on data: LS/EX") \
	MACRO(raw_LS_MT,			1024,	"RAW on data: LS/MT") \
	MACRO(raw_EX_MT,			2048,	"RAW on data: EX/MT") \
	MACRO(raw_MT_EX,			2048,	"RAW on data: MT/EX") \
	MACRO(raw_DSPLS_DSPLS,		512,	"RAW on data: DSPLS/DSPLS") \
	MACRO(noraw_LS_LS,			1024,	"No dependency: LS/LS") \
	MACRO(noraw_LS_EX,			1024,	"No dependency: LS/EX") \
	MACRO(raw_MT_LS_addr,		1024,	"RAW on address: MT/LS") \
	MACRO(raw_EX_LS_addr,		1024,	"RAW on address: EX/LS") \
	MACRO(raw_EX_LS_index,		1024,	"RAW on index: EX/LS") \
	MACRO(raw_LS_LS_addr,		1024,	"RAW on address: LS/LS") \
	MACRO(branch_bra,			1024,	"Branching: bra") \
	MACRO(branch_bra_cpuloop,	1024,	"Branching: bra (CPU loop)") \
	MACRO(darken_1,				512,	"Darken: 32-bit #1") \
	MACRO(darken_2,				512,	"Darken: 32-bit #2") \
	MACRO(darken_3,				256,	"Darken: +unrolled") \
	MACRO(darken_4,				256,	"Darken: +pipelined") \
	MACRO(double_read,			1024,	"Double read") \
	MACRO(double_incr_read,		1024,	"Double increment read") \
	MACRO(double_write,			1024,	"Double write") \
	MACRO(azur_p8_rgb565,		512,	"Azur: P8_RGB565 loop") \
	MACRO(azur_p8_rgb565a,		512,	"Azur: P8_RGB565A loop") \

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
	#define MACRO_RESULTS(name, count, str) int name;
	ALL_TESTS(MACRO_RESULTS)
};

/* Number of Iphi cycles total, and number of iterations */
static struct results r_cycles, r_iter;

static void table_gen(gtable *t, int row)
{
	#define MACRO_STR(name, count, str) str,
	static char const *names[] = {
		ALL_TESTS(MACRO_STR)
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

			#define MACRO_RUN(name, iter, str) {					\
				extern void perf_cpu_ ## name (void);				\
				r_cycles.name = Iphi_cycles(perf_cpu_ ## name);		\
				r_iter.name = iter;									\
			}
			ALL_TESTS(MACRO_RUN)

			table->widget.update = 1;
		}
	}

	gscreen_destroy(scr);
}
