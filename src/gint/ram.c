#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/cpu.h>

#include <gintctl/util.h>
#include <gintctl/gint.h>
#include <gintctl/assets.h>
#include <gintctl/ui.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* Byte-based memory detection functions */

static int writable(uint8_t volatile *mem)
{
	int save = *mem;
	*mem = save ^ 0xff;

	/* Read the written value and restore the pointed byte */
	int measured = *mem;
	*mem = save;

	/* The address is writable iff we succeeded in storing ~save */
	return (measured == (save ^ 0xff));
}

static int same_location(uint8_t volatile *m1, uint8_t volatile *m2)
{
	uint8_t s1=*m1, s2=*m2;

	*m1 = s1 ^ 0xf0;
	int equal1 = (*m2 == *m1);

	*m1 = s1 ^ 0x0f;
	int equal2 = (*m2 == *m1);

	*m1 = s1 ^ 0xff;
	int equal3 = (*m2 == *m1);

	*m1 = s1;
	*m2 = s2;

	return equal1 && equal2 && equal3;
}

/* Longword-based memory detection functions */

static int writable_lword(uint8_t volatile *mem_8)
{
	uint32_t volatile *mem = (void *)mem_8;

	uint32_t save = *mem;
	*mem = save ^ 0xffffff00;

	uint32_t measured = *mem;
	*mem = save;

	return (measured == (save ^ 0xffffff00));
}

static int same_location_lword(uint8_t volatile *m1_8, uint8_t volatile *m2_8)
{
	uint32_t volatile *m1 = (void *)m1_8, *m2 = (void *)m2_8;
	uint32_t s1=*m1, s2=*m2;

	*m1 = s1 ^ 0xffff0000;
	int equal1 = (*m2 == *m1);

	*m1 = s1 ^ 0x0000ff00;
	int equal2 = (*m2 == *m1);

	*m1 = s1 ^ 0xffffff00;
	int equal3 = (*m2 == *m1);

	*m1 = s1;
	*m2 = s2;

	return equal1 && equal2 && equal3;
}

/* Region size detection */

struct region {
	/* Region name (for display) [input] */
	char const *name;
	/* Region address [input] */
	uint32_t mem;
	/* Whether region supports only 32-bit access [input] */
	bool use_lword;
	/* How often to probe memory (1 in step_size bytes will be tested) */
	int step_size;
	/* Size of region [output] */
	uint32_t size;
	/* Reason why region is not larger [output] */
	int reason;
};

static void explore_region(struct region *r)
{
	uint8_t volatile *mem = (void *)r->mem;
	r->size = 0;
	r->reason = 0;

	cpu_atomic_start();

	while(r->size < (16 << 20))
	{
		int x = r->use_lword
			? writable_lword(mem + r->size)
			: writable(mem + r->size);
		if(!x)
		{
			r->reason = 1;
			break;
		}

		if(r->size > 0)
		{
			int y = r->use_lword
				? same_location_lword(mem, mem+r->size)
				: same_location(mem, mem+r->size);
			if(y)
			{
				r->reason = 2;
				break;
			}
		}

		r->size += r->step_size;
	}

	if(r->reason == 0)
		r->reason = 3;
	cpu_atomic_end();
}

/* Detailed 0xe50[01]xxxx search
   On the SH-4A this region contains the OLRAM. On the SH4AL-DSP it is supposed
   to contain nothing, but on the calculator the XRAM and YRAM (normally in P1)
   use part of the SH-4A OLRAM region. How exactly they wrap around is
   uncertain, so this analysis finds independent pages accurately. */
static void e500_search(int e500_pages[32])
{
	for(int i = 0; i < 32; i++) {
		uint8_t volatile *p1 = (void *)0xe5000000 + (i << 12);

		for(int j = 0; j < 32; j++) {
			uint8_t volatile *p2 = (void *)0xe5000000 + (j << 12);
			if(same_location(p1, p2)) {
				e500_pages[i] = j;
				break;
			}
		}
	}
}

static void table_gen(gtable *t, int row, struct region const *regions)
{
	struct region const *r = &regions[row];
	char const *reasons[] = {
		"Not tested",
		_("Read-only", "Not writable"),
		_("Loops", "Wraps around"),
		_("More?", "Maybe larger!"),
	};

	GUNUSED char c2[16], c3[16], c4[16] = "-";
	sprintf(c2, "%08X", r->mem);

#if GINT_RENDER_MONO
	if(r->reason != 0)
		sprintf(c4, "%dk", r->size >> 10);
	gtable_provide(t, r->name, c2, c4, reasons[r->reason]);
#else
	sprintf(c3, "%d", r->use_lword ? 32 : 8);
	if(r->reason != 0)
		sprintf(c4, "%d %s", r->size, r->size >= (1000000) ? "B" : "bytes");
	gtable_provide(t, r->name, c2, c3, c4, reasons[r->reason]);
#endif
}

/* gintctl_gint_ram(): Determine the size of some memory areas */
void gintctl_gint_ram(void)
{
	struct region r[] = {
		{ "ILRAM", 0xe5200000, false,    4, /**/ 0, 0 },
		{ "XRAM",  0xe5007000, false,    4, /**/ 0, 0 },
		{ "YRAM",  0xe5017000, false,    4, /**/ 0, 0 },
		{ "PRAM0", 0xfe200000, true,    32, /**/ 0, 0 },
		{ "XRAM0", 0xfe240000, true,    32, /**/ 0, 0 },
		{ "YRAM0", 0xfe280000, true,    32, /**/ 0, 0 },
		{ "PRAM1", 0xfe300000, true,    32, /**/ 0, 0 },
		{ "XRAM1", 0xfe340000, true,    32, /**/ 0, 0 },
		{ "YRAM1", 0xfe380000, true,    32, /**/ 0, 0 },
		{ "RSRAM", 0xfd800000, false,    4, /**/ 0, 0 },
		{ "URAM",  0xa55f0000, false,    4, /**/ 0, 0 },
		{ "RAM",   0xac000000, false, 1024, /**/ 0, 0 },
	};

	/* Region count (for the scrolling list on fx-9860G) */
	GUNUSED int region_count = 9;

	/* Detailed 16-page e500 search */
	int e500_pages[32];
	e500_search(e500_pages);

	gscreen *s = gscreen_create2("RAM discovery", &img_opt_gint_ram,
		"On-chip and external RAM discovery",
		"@ILRAM;@XYRAM;@DSP;@?;#BANKS;#E500", NULL);
	gintctl_scene_push(s);

	// RAM region table

	gtable *table = gtable_create(_(4,5), table_gen, (void *)r, NULL);
	gtable_set_rows(table, sizeof r / sizeof r[0]);
#if GINT_RENDER_MONO
	gtable_set_column_titles(table, "Area", "Address", "Size", "At end");
	gtable_set_column_sizes(table, 25, 36, 20, 38);
	gtable_set_font(table, &font_mini);
#else
	gtable_set_column_titles(table, "Area", "Address", "AS", "Size", "At end");
	gtable_set_column_sizes(table, 4, 6, 2, 8, 10);
	gtable_set_row_height(table, 12);
#endif
	gscreen_add_tab(s, table, table);

	// E500 breakdown table

	jlabel *label_e500 = jlabel_create("<e500>", NULL);
	jlabel_set_font(label_e500, _(&font_mini, dfont_default()));

	char str_e500[512] = _("e50xx000", "e50xxxxx") " pages:\n";
	int n = strlen(str_e500);
	for(int i = 0; i < 32; i++)
		n += sprintf(str_e500 + n, _(" ", "  ") "%02X%s:%02d%s",
			i, _("", "000"), e500_pages[i], ((i & 3) == 3) ? "\n" : "");
	jlabel_set_text(label_e500, str_e500);
	gscreen_add_tab(s, label_e500, NULL);
	gscreen_set_tab_title_visible(s, 1, _(false, true));

	// Event loop

	while(true) {
		jevent e = jscene_run(gintctl_scene());
		if(jevent_is_press(e, KEY_EXIT))
			break;

		if(e.type == JFKEYS_TRIGGERED && e.data == 0)
			explore_region(&r[0]);
		if(e.type == JFKEYS_TRIGGERED && e.data == 1) {
			explore_region(&r[1]);
			explore_region(&r[2]);
		}
		if(e.type == JFKEYS_TRIGGERED && e.data == 2) {
			explore_region(&r[3]);
			explore_region(&r[4]);
			explore_region(&r[5]);
			explore_region(&r[6]);
			explore_region(&r[7]);
			explore_region(&r[8]);
		}
		if(e.type == JFKEYS_TRIGGERED && e.data == 3) {
			explore_region(&r[9]);
			explore_region(&r[10]);
			explore_region(&r[11]);
		}
		if(e.type == JFKEYS_TRIGGERED && e.data == 4)
			gscreen_show_tab(s, gscreen_current_tab(s) == 1 ? 0 : 1);
		if(e.type == JFKEYS_TRIGGERED && e.data == 5)
			gscreen_show_tab(s, gscreen_current_tab(s) == 2 ? 0 : 2);

		table->widget.update = 1;
	}
}
