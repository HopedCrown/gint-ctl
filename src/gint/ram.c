#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/cpu.h>
#include <gint/mpu/spu.h>
#define SPU SH7305_SPU

#include <gintctl/util.h>
#include <gintctl/gint.h>
#include <gintctl/assets.h>
#include <gintctl/ui.h>
#include <justui/jpainted.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

//---
// RAM size discovery
//---

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

//---
// e500-area page search
//---

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

//---
// SPU2 RAM banking settings
//---

static int bank_number(volatile uint32_t *area)
{
	/* Check that the address is writable */
	uint32_t save = *area;
	*area = save ^ 0xffffff00;
	uint32_t read = *area;
	*area = save;

	if(read != (save ^ 0xffffff00)) return -1;
	return save >> 24;
}

static void save_and_setup(volatile uint32_t *area, uint32_t *save, int pages)
{
	for(int i = 0; i < pages; i++)
	{
		save[i] = area[i * 0x2000];
		area[i * 0x2000] = (i << 24);
	}
}

static void restore(volatile uint32_t *area, uint32_t *save, int pages)
{
	for(int i = 0; i < pages; i++)
	{
		area[i * 0x2000] = save[i];
	}
}

#if GINT_RENDER_MONO
static void render_area(int x, int y, char const *name, u32 *area)
{
	dprint(x, y, C_BLACK, "%s:", name);

	for(int b = 0; b < 7; b++) {
		int bank = bank_number(area);
		dprint(x+27+6*b, y, C_BLACK, bank >= 0 ? "%d" : "-", bank);
		area += 0x2000;
		if(bank < 0)
			break;
	}
}

static void render_bank_pair(int x, int y, char const *name,
	u32 volatile *BANKC0, u32 volatile *BANKC1, int pages, int focus)
{
	dtext(x, y, C_BLACK, name);
	dprint(x, y+ 7, C_BLACK, "C0=%02X", *BANKC0);
	dprint(x, y+14, C_BLACK, "C1=%02X", *BANKC1);

	for(int bank_num = 0; bank_num < 2; bank_num++) {
		for(int p = 0; p < pages; p++) {
			int active = (bank_num == 0 ? *BANKC0 : *BANKC1) & (1 << p);

			int rx = x + 21 + 6 * p;
			int ry = y + 6 * (bank_num + 1);

			int fill = active ? C_BLACK : C_WHITE;
			drect_border(rx, ry, rx+6, ry+6, fill, 1, C_BLACK);

			if(!bank_num && focus == p) {
				dline(rx+3, ry-1, rx+1, ry-3, C_BLACK);
				dline(rx+3, ry-1, rx+5, ry-3, C_BLACK);
			}
		}
	}
}

static void render_spubanks(int x, int y, int *cursor)
{
	u32 *PRAM0 = (void *)0xfe200000;
	u32 *XRAM0 = (void *)0xfe240000;
	u32 *YRAM0 = (void *)0xfe280000;
	u32 *PRAM1 = (void *)0xfe300000;
	u32 *XRAM1 = (void *)0xfe340000;
	u32 *YRAM1 = (void *)0xfe380000;

	int cur_bank = (*cursor >= 5);
	int cur_page = *cursor - 5 * cur_bank;

	extern font_t font_mini;
	font_t const *old_font = dfont(&font_mini);

	render_area(x+75*0, y+6*0, "PRAM0", PRAM0);
	render_area(x+75*0, y+6*1, "XRAM0", XRAM0);
	render_area(x+75*1, y+6*0, "YRAM0", YRAM0);
	render_area(x+75*0, y+6*2, "PRAM1", PRAM1);
	render_area(x+75*0, y+6*3, "XRAM1", XRAM1);
	render_area(x+75*1, y+6*2, "YRAM1", YRAM1);

	render_bank_pair(x, y+6*4+2, "PBANK", &SPU.PBANKC0, &SPU.PBANKC1, 5,
		(cur_bank == 0) ? cur_page : -1);
	render_bank_pair(x+59, y+6*4+2, "XBANK", &SPU.XBANKC0, &SPU.XBANKC1, 7,
		(cur_bank == 1) ? cur_page : -1);

	dfont(old_font);
}
#endif

#if GINT_RENDER_RGB
static void render_area(int x, int y, char const *name, uint32_t *area)
{
	dtext(x+8, y, C_BLACK, name);
	dprint(x+64, y, C_BLACK, "%08X", (u32)area);

	for(int p = 0; true; p++) {
		int bank = bank_number(area);
		if(bank < 0) break;

		dprint(x-6+row_x(20+4*p), y, C_BLACK, "%d", bank);
		area += 0x2000;
	}
}

static void render_bank(int x, int y, char const *name, u32 volatile *bank,
	int pages, int dsp, int focus)
{
	dprint(x +  6, y, C_BLACK, "%s = %02X", name, *bank);

	int fx = -1, fy = -1;
	int size = 12;

	for(int p = 0; p < pages; p++)
	{
		int active = *bank & (1 << p);

		int rx = x + 108 + size * p - 2;
		int ry = y + 2 * (dsp == 0) - 4;

		int fill = C_RGB(28,28,28);
		if(active) fill = dsp ? C_RGB(31,24,0) : C_GREEN;
		drect_border(rx, ry, rx+size, ry+size, fill, 1, C_RGB(16,16,16));

		if(focus == p)
			fx = rx, fy = ry;
	}

	if(focus >= 0)
		drect_border(fx, fy, fx+size, fy+size, C_NONE, 1, C_BLACK);
}

static void render_spubanks(int x, int y, int *cursor)
{
	uint32_t *PRAM0 = (void *)0xfe200000;
	uint32_t *XRAM0 = (void *)0xfe240000;
	uint32_t *YRAM0 = (void *)0xfe280000;
	uint32_t *PRAM1 = (void *)0xfe300000;
	uint32_t *XRAM1 = (void *)0xfe340000;
	uint32_t *YRAM1 = (void *)0xfe380000;

	int cur_bank = (*cursor >= 5);
	int cur_page = *cursor - 5 * cur_bank;

	dtext(x+8, y, C_BLACK, "Area:");
	dtext(x+64, y, C_BLACK, "Address:");

	for(int p = 0; p < 7; p++)
		dprint(x-6+row_x(20+4*p), y, C_BLACK, "%d:", p*32);

	render_area(x, y+14*1, "PRAM0", PRAM0);
	render_area(x, y+14*2, "XRAM0", XRAM0);
	render_area(x, y+14*3, "YRAM0", YRAM0);
	render_area(x, y+14*4, "PRAM1", PRAM1);
	render_area(x, y+14*5, "XRAM1", XRAM1);
	render_area(x, y+14*6, "YRAM1", YRAM1);

	dtext(x, y+14*8, C_BLACK, "Bank settings for DSP0 and DSP1:");
	render_bank(x, y+14*9, "PBANKC0", &SPU.PBANKC0, 5, 0,
		(cur_bank == 0) ? cur_page : -1);
	render_bank(x, y+14*10, "PBANKC1", &SPU.PBANKC1, 5, 1,
		(cur_bank == 0) ? cur_page : -1);

	render_bank(x+185, y+14*9, "XBANKC0", &SPU.XBANKC0, 7, 0,
		(cur_bank == 1) ? cur_page : -1);
	render_bank(x+185, y+14*10, "XBANKC1", &SPU.XBANKC1, 7, 1,
		(cur_bank == 1) ? cur_page : -1);
}
#endif

//---
// Main interface
//---

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

	/* SPU bank saves */
	u32 saves[6][7];
	save_and_setup((void *)r[3].mem, saves[0], 5);
	save_and_setup((void *)r[4].mem, saves[1], 7);
	save_and_setup((void *)r[5].mem, saves[2], 2);
	save_and_setup((void *)r[6].mem, saves[3], 5);
	save_and_setup((void *)r[7].mem, saves[4], 7);
	save_and_setup((void *)r[8].mem, saves[5], 2);

	gscreen *s = gscreen_create2("RAM discovery", &img_opt_gint_ram,
		"On-chip and external RAM discovery",
		"@ILRAM;@XYRAM;@DSP;@?;#E500;#BANKS", NULL);
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

	// SPU banking settings

	int cursor = 0;
	jpainted *spubanks =
		jpainted_create(render_spubanks, &cursor, 10, 10, NULL);
	gscreen_add_tab(s, spubanks, NULL);
	// Desired tab title:
	// _("SPU bank pages", "SPU memory banks for {P,X,Y}RAM{0,1}")

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

		/* Manual input on the SPU banking tab */
		if(gscreen_current_tab(s) == 2) {
			if(jevent_is_press(e, KEY_LEFT))
				cursor -= (cursor > 0);
			if(jevent_is_press(e, KEY_RIGHT))
				cursor += (cursor + 1 < 12);

			int cur_bank = (cursor >= 5);
			int cur_page = cursor - 5 * cur_bank;

			bool is_switch =
				jevent_is_press(e, KEY_EXE) || jevent_is_press(e, KEY_SHIFT);
			if(is_switch && cur_bank == 0) {
				SPU.PBANKC0 ^= (1 << cur_page);
				SPU.PBANKC1 ^= (1 << cur_page);
			}
			if(is_switch && cur_bank == 1) {
				SPU.XBANKC0 ^= (1 << cur_page);
				SPU.XBANKC1 ^= (1 << cur_page);
			}

			spubanks->widget.update = 1;
		}

		table->widget.update = 1;
	}

	/* Restore the values we saved before altering page data */
	restore((void *)r[3].mem, saves[0], 5);
	restore((void *)r[4].mem, saves[1], 7);
	restore((void *)r[5].mem, saves[2], 2);
	restore((void *)r[6].mem, saves[3], 5);
	restore((void *)r[7].mem, saves[4], 7);
	restore((void *)r[8].mem, saves[5], 2);
}
