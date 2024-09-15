#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/mpu/spu.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

#define SPU SH7305_SPU

//---
// Logic and save/restore RAM data
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

static void render_widget(int x, int y, int cur_bank, int cur_page)
{
	uint32_t *PRAM0 = (void *)0xfe200000;
	uint32_t *XRAM0 = (void *)0xfe240000;
	uint32_t *YRAM0 = (void *)0xfe280000;
	uint32_t *PRAM1 = (void *)0xfe300000;
	uint32_t *XRAM1 = (void *)0xfe340000;
	uint32_t *YRAM1 = (void *)0xfe380000;

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

/* gintctl_gint_spuram(): SPU memory access, banking, and DMA */
void gintctl_gint_spuram(void)
{
	extern int spu_zero(void);
	spu_zero();
	int key = 0;

	uint32_t *PRAM0 = (void *)0xfe200000;
	uint32_t *XRAM0 = (void *)0xfe240000;
	uint32_t *YRAM0 = (void *)0xfe280000;
	uint32_t *PRAM1 = (void *)0xfe300000;
	uint32_t *XRAM1 = (void *)0xfe340000;
	uint32_t *YRAM1 = (void *)0xfe380000;

	/* Save values for all banks for all areas */
	uint32_t saves[6][8];
	save_and_setup(PRAM0, saves[0], 5);
	save_and_setup(XRAM0, saves[1], 7);
	save_and_setup(YRAM0, saves[2], 4);
	save_and_setup(PRAM1, saves[3], 5);
	save_and_setup(XRAM1, saves[4], 7);
	save_and_setup(YRAM1, saves[5], 4);

	int cursor = 0;
	int cur_bank = 0; // derived from cursor
	int cur_page = 0; // derived from cursor

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#if GINT_RENDER_RGB
		row_title("SPU memory: {P,X,Y}RAM{0,1}");
		row_print(1, 1, "Pages layout in memory (offsets in kiB):");
		render_widget(6, 36, cur_bank, cur_page);
		#endif

		dupdate();
		key = getkey_opt(GETKEY_DEFAULT & ~GETKEY_MOD_SHIFT, NULL).key;

		if(key == KEY_LEFT)
			cursor -= (cursor > 0);
		if(key == KEY_RIGHT)
			cursor += (cursor + 1 < 12);
		cur_bank = (cursor >= 5);
		cur_page = cursor - 5 * cur_bank;

		if((key == KEY_EXE || key == KEY_SHIFT) && cur_bank == 0)
		{
			SPU.PBANKC0 ^= (1 << cur_page);
			SPU.PBANKC1 ^= (1 << cur_page);
		}
		if((key == KEY_EXE || key == KEY_SHIFT) && cur_bank == 1)
		{
			SPU.XBANKC0 ^= (1 << cur_page);
			SPU.XBANKC1 ^= (1 << cur_page);
		}
	}

	/* Restore the values we saved before altering page data */
	restore(PRAM0, saves[0], 5);
	restore(XRAM0, saves[1], 7);
	restore(YRAM0, saves[2], 4);
	restore(PRAM1, saves[3], 5);
	restore(XRAM1, saves[4], 7);
	restore(YRAM1, saves[5], 4);
}
