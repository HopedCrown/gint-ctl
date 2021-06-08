#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/util.h>
#include <gintctl/gint.h>

#include <stdio.h>
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
	/* Size of region [output] */
	uint32_t size;
	/* Reason why region is not larger [output] */
	int reason;
};

static void explore_region(struct region *r)
{
	uint8_t volatile *mem = (void *)r->mem;
	r->size = 0;

	while(r->size < (1 << 20))
	{
		int x = r->use_lword
			? writable_lword(mem + r->size)
			: writable(mem + r->size);

		r->reason = 1;
		if(!x) return;

		if(r->size > 0)
		{
			int y = r->use_lword
				? same_location_lword(mem, mem+r->size)
				: same_location(mem, mem+r->size);
			r->reason = 2;
			if(y) return;
		}

		/* In PXYRAM, skip some longwords to go faster */
		r->size += r->use_lword ? 32 : 4;
	}

	r->reason = 3;
}

#ifdef FX9860G
static void show_region(int row, struct region *r)
{
	/* Out-of-bounds rows */
	if(row < 1 || row > 9 || (row == 1 && r)) return;

	extern font_t font_mini;
	font_t const *old_font = dfont(&font_mini);
	int y = (row - 1) * 6 + 2 * (row > 1);

	if(!r)
	{
		dprint( 1, y, C_BLACK, "Area");
		dprint(26, y, C_BLACK, "Address");
		dprint(62, y, C_BLACK, "Size");
		dprint(82, y, C_BLACK, "At end");
		dfont(old_font);
		return;
	}

	char const *reasons[] = { "Not tested", "Read-only", "Loops", "" };

	dprint( 1, y, C_BLACK, "%s", r->name);
	dprint(26, y, C_BLACK, "%08X", r->mem);

	if(r->reason != 0)
		dprint(62, y, C_BLACK, "%dk", r->size >> 10);
	else
		dprint(62, y, C_BLACK, "-");

	dprint(82, y, C_BLACK, "%s", reasons[r->reason]);
	dfont(old_font);
}
#endif

#ifdef FXCG50
static void show_region(int y, struct region *r)
{
	char const *reasons[] = {
		"Not tested",
		"Not writable",
		"Wraps around",
		"Maybe larger!",
	};

	if(!r)
	{
		row_print(y,  2, "Area");
		row_print(y,  9, "Address");
		row_print(y, 18, "AS");
		row_print(y, 22, "Size");
		row_print(y, 35, "At end");
		return;
	}

	row_print(y,  2, "%s", r->name);
	row_print(y,  9, "%08X", r->mem);
	row_print(y, 18, "%d", r->use_lword ? 32 : 8);
	row_print(y, 22, "%d bytes", r->size);
	row_print(y, 35, reasons[r->reason]);
}
#endif

/* gintctl_gint_ram(): Determine the size of some memory areas */
void gintctl_gint_ram(void)
{
	struct region r[] = {
		{ "ILRAM", 0xe5200000, false, 0, 0 },
		{ "XRAM",  0xe5007000, false, 0, 0 },
		{ "YRAM",  0xe5017000, false, 0, 0 },
		{ "PRAM0", 0xfe200000, true,  0, 0 },
		{ "XRAM0", 0xfe240000, true,  0, 0 },
		{ "YRAM0", 0xfe280000, true,  0, 0 },
		{ "PRAM1", 0xfe300000, true,  0, 0 },
		{ "XRAM1", 0xfe340000, true,  0, 0 },
		{ "YRAM1", 0xfe380000, true,  0, 0 },
		{ "X_P2",  0xa5007000, false, 0, 0 },
		{ "URAM",  0xa55f0000, false, 0, 0 },
		{ NULL },
	};

	/* Region count (for the scrolling list on fx-9860G) */
	GUNUSED int region_count = 9;
	/* List scroll no fx-9860G */
	GUNUSED int scroll = spu_zero();

	key_event_t ev;
	int key = 0;
	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		show_region(1, NULL);
		dhline(6, C_BLACK);
		for(int i = 0; i < region_count; i++)
		{
			show_region(i+2-scroll, &r[i]);
		}
		scrollbar_px(/* view */ 8, 54, /* range */ 0, region_count,
			/* visible */ scroll, 8);

		extern bopti_image_t img_opt_gint_ram;
		dimage(0, 56, &img_opt_gint_ram);
		#endif

		#ifdef FXCG50
		row_title("On-chip memory discovery");
		show_region(1, NULL);
		for(int i = 0; r[i].name; i++)
		{
			show_region(i+2, &r[i]);
		}

		fkey_button(1, "ILRAM");
		fkey_button(2, "XYRAM");
		fkey_button(3, "DSP0");
		fkey_button(4, "DSP1");
		#endif

		dupdate();

		ev = getkey();
		key = ev.key;
		if(key == KEY_F1)
		{
			explore_region(&r[0]);
		}
		if(key == KEY_F2)
		{
			explore_region(&r[1]);
			explore_region(&r[2]);
		}
		if(key == KEY_F3)
		{
			explore_region(&r[3]);
			explore_region(&r[4]);
			explore_region(&r[5]);
		}
		if(key == KEY_F4)
		{
			explore_region(&r[6]);
			explore_region(&r[7]);
			explore_region(&r[8]);
		}
		if(key == KEY_F5)
		{
			explore_region(&r[9]);
			explore_region(&r[10]);
		}

		#ifdef FX9860G
		int scroll_max = region_count - 8;
		if(key == KEY_UP)
		{
			if(ev.shift || keydown(KEY_SHIFT)) scroll=0;
			else if(scroll > 0) scroll--;
		}
		if(key == KEY_DOWN)
		{
			if(ev.shift || keydown(KEY_SHIFT)) scroll=scroll_max;
			else if(scroll < scroll_max) scroll++;
		}
		#endif
	}
}
