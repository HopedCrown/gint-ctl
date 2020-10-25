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

#ifdef FXCG50
static void render_header(int y, int bank_count)
{
	dtext(row_x(2),  row_y(y), C_BLACK, "Area:");
	dtext(row_x(9),  row_y(y), C_BLACK, "Address:");

	for(int b = 0; b < bank_count; b++)
	{
		row_print(y, 20+4*b, "%d:", b*32);
	}
}

static void render_area(int y, char const *name, uint32_t *area)
{
	row_print(y, 2, "%s", name);
	row_print(y, 9, "%08X", (uint32_t)area);

	int b = 0, bank = 0;
	while(1)
	{
		bank = bank_number(area);
		if(bank == -1) break;

		row_print(y, 20+4*b, "%d", bank);
		area += 0x2000;
		b++;
	}
}

static void render_bank(int y, char const *name, volatile uint32_t *bank,
	int banks, int dsp, int focus)
{
	row_print(y, 2, "%s", name);
	row_print(y, 10, "= %02X", *bank);

	for(int b = 0; b < banks; b++)
	{
		int active = *bank & (1 << b);

		int w  = row_x(3) - row_x(0);
		int x  = row_x(20 + 4*b) - 2;
		int h  = row_y(1) - row_y(0) - 2;
		int ry = row_y(y) + 2*(dsp==0) - 4;

		int fill = C_RGB(24,24,24);
		if(active) fill = dsp ? C_RGB(31,24,0) : C_GREEN;
		int border = (focus == b) ? C_BLACK : C_RGB(16,16,16);
		drect_border(x, ry, x+w, ry+h, fill, 1, border);
	}
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

	int cur_bank = 0;
	int cur_page = 0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FXCG50
		row_title("SPU memory: PRAM0, XRAM0, YRAM0, YRAM");
		row_print(1, 1, "Pages layout in memory (offsets in kiB):");

		render_header(2, 7);
		render_area(3, "PRAM0", PRAM0);
		render_area(4, "XRAM0", XRAM0);
		render_area(5, "YRAM0", YRAM0);
		render_area(6, "PRAM1", PRAM1);
		render_area(7, "XRAM1", XRAM1);
		render_area(8, "YRAM1", YRAM1);

		row_print(9, 1, "Bank settings for DSP0 and DSP1:");
		render_bank(10, "PBANKC0", &SPU.PBANKC0, 5, 0,
			(cur_bank == 0) ? cur_page : -1);
		render_bank(11, "PBANKC1", &SPU.PBANKC1, 5, 1,
			(cur_bank == 0) ? cur_page : -1);
		render_bank(12, "XBANKC0", &SPU.XBANKC0, 7, 0,
			(cur_bank == 1) ? cur_page : -1);
		render_bank(13, "XBANKC1", &SPU.XBANKC1, 7, 1,
			(cur_bank == 1) ? cur_page : -1);

		fkey_button(1, "SWITCH");
		#endif

		dupdate();
		key = getkey().key;

		if(key == KEY_LEFT && cur_page > 0)
			cur_page--;
		if(key == KEY_RIGHT && cur_page < (cur_bank ? 7 : 5) - 1)
			cur_page++;
		if(key == KEY_UP && cur_bank == 1 && cur_page < 5)
			cur_bank--;
		if(key == KEY_DOWN && cur_bank == 0)
			cur_bank++;

		if(key == KEY_F1 && cur_bank == 0)
		{
			SPU.PBANKC0 ^= (1 << cur_page);
			SPU.PBANKC1 ^= (1 << cur_page);
		}
		if(key == KEY_F1 && cur_bank == 1)
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
