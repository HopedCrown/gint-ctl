#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <gint/mmu.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

#define PAGE_USED    0x01
#define PAGE_MAPPED  0x02
#define C_ C_BLACK, C_NONE
#define TLB_VIEW_MAX (64 - TLB_VIEW)

#ifdef FXCG50
#define PAGE_COUNT 0x200
#define SQUARE_WIDTH 8
#define SQUARE_HEIGHT 8
#define LINE_SIZE 32
#define TLB_VIEW 12

static void draw_rom_cell(int x, int y, int status)
{
	uint16_t colors[4] = {
		C_RGB(24,24,24),	/* Unused and unmapped */
		C_WHITE,		/* Used, but currently unmapped */
		C_RGB(31,24,0),		/* Unused but still mapped?! */
		C_RGB(0,31,0),		/* Used and currently mapped */
	};
	uint16_t border =
		status == -1 ? C_RGB(0,0,31) :
		status == -2 ? C_RGB(31,0,0) :
		C_BLACK;

	if(status >= 0 && status < 4)
		drect(x, y, x+SQUARE_WIDTH, y+SQUARE_HEIGHT, colors[status]);

	dline(x, y, x+SQUARE_WIDTH, y, border);
	dline(x, y, x, y+SQUARE_HEIGHT, border);
	dline(x+SQUARE_WIDTH, y, x+SQUARE_WIDTH, y+SQUARE_HEIGHT, border);
	dline(x, y+SQUARE_HEIGHT, x+SQUARE_WIDTH, y+SQUARE_HEIGHT, border);
}
#endif

#ifdef FX9860G
#define PAGE_COUNT 0x80
#define SQUARE_WIDTH 4
#define SQUARE_HEIGHT 4
#define LINE_SIZE 16
#define TLB_VIEW 8

static void draw_rom_cell(int x, int y, int status)
{
	extern bopti_image_t img_tlb_cells;
	dsubimage(x, y, &img_tlb_cells, (status+2)*(SQUARE_WIDTH+2), 0,
		SQUARE_WIDTH+1, SQUARE_HEIGHT+1, DIMAGE_NONE);
}
#endif

static int rom_cell_x(int page_number)
{
	int x = (page_number % LINE_SIZE) * SQUARE_WIDTH;
	x += 3 * ((page_number % LINE_SIZE) >> 3);
	return x;
}
static int rom_cell_y(int page_number)
{
	return (page_number / LINE_SIZE) * SQUARE_HEIGHT;
}

#ifdef FX9860G
void triangle_up(int y)
{
	int x=118;
	dpixel(x+2, y, C_BLACK);
	dline(x+1, y+1, x+3, y+1, C_BLACK);
	dline(x,   y+2, x+4, y+2, C_BLACK);
}
void triangle_down(int y)
{
	int x=118;
	dline(x,   y,   x+4, y,   C_BLACK);
	dline(x+1, y+1, x+3, y+1, C_BLACK);
	dpixel(x+2, y+2, C_BLACK);
}
#endif

#ifdef FXCG50
void triangle_up(int y)
{
	int x=370;
	dpixel(x+3, y, C_BLACK);
	dline(x+2, y+1, x+4, y+1, C_BLACK);
	dline(x+1, y+2, x+5, y+2, C_BLACK);
	dline(x,   y+3, x+6, y+3, C_BLACK);
	dline(x,   y+4, x+6, y+4, C_BLACK);
}
void triangle_down(int y)
{
	int x=370;
	dline(x,   y,   x+6, y,   C_BLACK);
	dline(x,   y+1, x+6, y+1, C_BLACK);
	dline(x+1, y+2, x+5, y+2, C_BLACK);
	dline(x+2, y+3, x+4, y+3, C_BLACK);
	dpixel(x+3, y+4, C_BLACK);
}
#endif

static void explore_pages(uint8_t *pages, uint32_t *next_miss)
{
	extern uint32_t srom;

	for(uint p = 0; p < PAGE_COUNT; p++)
	{
		pages[p] = ((p << 12) < (uint32_t)&srom) ? PAGE_USED : 0;
	}

	for(uint E = 0; E < 64; E++)
	{
		utlb_addr_t addr = *utlb_addr(E);
		utlb_data_t data = *utlb_data(E);

		uint32_t src = addr.VPN << 10;
		int valid = (addr.V != 0) && (data.V != 0);
		int size = (data.SZ1 << 1) | data.SZ2;
		if(!valid || size != 1) continue;

		int p = (src - 0x00300000) >> 12;
		pages[p] |= PAGE_MAPPED;
	}

	*next_miss = 0xffffffff;
	for(uint p = 0; p < PAGE_COUNT; p++)
	{
		if((pages[p] & PAGE_USED) && !(pages[p] & PAGE_MAPPED))
		{
			*next_miss = 0x00300000 + (p << 12);
			break;
		}
	}
}

#ifdef FXCG50
void show_utlb(int row, int E)
{
	if(E == -1)
	{
		row_print(row, 2, "ID");
		row_print(row, 5,  "Virtual");
		row_print(row, 14, "Physical");
		row_print(row, 23, "Size");
		row_print(row, 28, "Mode");
		row_print(row, 34, "SH");
		row_print(row, 37, "WT");
		row_print(row, 40, "C");
		row_print(row, 42, "D");
		row_print(row, 44, "V");
		return;
	}

	utlb_addr_t addr = *utlb_addr(E);
	utlb_data_t data = *utlb_data(E);

	uint32_t src = addr.VPN << 10;
	uint32_t dst = data.PPN << 10;

	int valid = (addr.V != 0) && (data.V != 0);
	int size = (data.SZ1 << 1) | data.SZ2;
	char const *size_str[] = { "1k", "4k", "64k", "1M" };
	char const *access_str[] = { "K:r", "K:rw", "U:r", "U:rw" };

	uint16_t fg = valid ? C_BLACK : C_RGB(24,24,24);
	#define fg_ fg, C_NONE

	row_print_color(row, 2,  fg_, "%d", E);
	row_print_color(row, 5,  fg_, "%08X", src);
	row_print_color(row, 14, fg_, "%08X", dst);
	row_print_color(row, 23, fg_, "%s", size_str[size]);
	row_print_color(row, 28, fg_, "%s", access_str[data.PR]);
	row_print_color(row, 34, fg_, "%s", (data.SH ? "SH" : "-"));
	row_print_color(row, 37, fg_, "%s", (data.WT ? "WT" : "CB"));
	row_print_color(row, 40, fg_, "%s", (data.C ? "C" : "-"));
	row_print_color(row, 42, fg_, "%s", (addr.D || data.D ? "D" : "-"));
	row_print_color(row, 44, fg_, "%s", (addr.V && data.V ? "V" : "-"));
}
#endif

#ifdef FX9860G
void show_utlb(int row, int E)
{
	extern font_t font_hexa;
	font_t const *old_font = dfont(&font_hexa);
	int y = (row - 1) * 6;

	if(E == -1)
	{
		dprint( 1, y, C_, "ID");
		dprint(12, y, C_, "Virtual");
		dprint(47, y, C_, "Physical");
		dprint(82, y, C_, "Len");
		dprint(98, y, C_, "Mode");

		dfont(old_font);
		return;
	}

	utlb_addr_t addr = *utlb_addr(E);
	utlb_data_t data = *utlb_data(E);

	uint32_t src = addr.VPN << 10;
	uint32_t dst = data.PPN << 10;

	int valid = (addr.V != 0) && (data.V != 0);
	int size = (data.SZ1 << 1) | data.SZ2;
	char const *size_str[] = { "1k", "4k", "64k", "1M" };
	char const *access_str[] = { "K:r", "K:rw", "U:r", "U:rw" };

	dprint( 1, y, C_, "%d", E);

	if(valid)
	{
		dprint(12, y, C_, "%08X", src);
		dprint(47, y, C_, "%08X", dst);
		dprint(82, y, C_, "%s", size_str[size]);
		dprint(98, y, C_, "%s", access_str[data.PR]);
	}

	dfont(old_font);
}
#endif

static void draw(int tab, uint8_t *pages, uint32_t next_miss, int tlb_scroll)
{
	extern uint32_t srom;
	uint32_t rom_size = (uint32_t)&srom;
	int rom_pages = (rom_size + (1 << 12)-1) >> 12;

	#ifdef FX9860G
	if(tab != 2)
	#endif
	row_title(_("TLB management", "TLB miss handler and TLB management"));

	if(tab == 0)
	{
		row_print(_(2,1), 1, _("srom=%x (%d pages)",
			"Size of ROM sections: %08X (%d pages of 4k)"),
			rom_size, rom_pages);

		for(uint p=0, y=_(19,36); p < PAGE_COUNT; p += 2*LINE_SIZE)
		{
			dprint(_(4,18), y, C_, _("%06x","%08X"),
				0x00300000 + (p << 12));
			y += 2*SQUARE_HEIGHT;
		}

		for(uint p = 0; p < PAGE_COUNT; p++)
		{
			draw_rom_cell(_(44,88) + rom_cell_x(p),
				_(20,36) + rom_cell_y(p), pages[p]);
		}

		#ifdef FXCG50
		if(next_miss != 0xffffffff)
		{
			uint p = (next_miss - 0x00300000) >> 12;
			draw_rom_cell(88+rom_cell_x(p), 36+rom_cell_y(p), -1);
			row_print(12, 1, "Next page to load: %08X", next_miss);
		}
		else
		{
			row_print(12, 1, "No page left to load!");
		}
		row_print(13, 1, "See INFO tab for details.");
		#endif
	}

	else if(tab == 1)
	{
		row_print(_(2,1), 1, "Legend:");
		draw_rom_cell(_(8,18),   _(17,36), 0);
		draw_rom_cell(_(64,200), _(17,36), 1);
		draw_rom_cell(_(8,18),   _(25,50), 2);
		draw_rom_cell(_(64,200), _(25,50), 3);

		dprint(_(16,30),  _(16,36), C_,
			_("Unused", "Unused in add-in"));
		dprint(_(72,212), _(16,36), C_,
			_("Unmapped", "Currently unmapped"));
		dprint(_(16,30),  _(24,50), C_,
			_("Mapped?", "Strangely mapped?!"));
		dprint(_(72,212), _(24,50), C_,
			_("Mapped", "Currently mapped"));

		#ifdef FXCG50
		draw_rom_cell(18, 64, -1);
		dprint(30, 64, C_, "Next page to load");

		row_print(6, 1,
			"The MISS key will load an unmapped page to TLB by");
		row_print(7, 1,
			"performing a TLB miss, which calls %%003 on fx9860g");
		row_print(8, 1,
			"and %%00c on fxcg50. The TIMER key performs a TLB");
		row_print(9, 1,
			"miss from a timer callback.");
		row_print(11, 1,
			"Sometimes the page might be loaded by code or");
		row_print(12, 1,
			"data before the intended TLB miss occurs.");
		#endif

		#ifdef FX9860G
		if(next_miss != 0xffffffff)
			row_print(5, 1, "Next load: %x", next_miss);
		else
			row_print(5, 1, "No page left to load!");
		row_print(6, 1, "MISS:  TLB miss");
		row_print(7, 1, "TIMER: From timer");
		#endif
	}

	else if(tab == 2)
	{
		show_utlb(1, -1);
		for(int i = 0; i < TLB_VIEW; i++)
			show_utlb(i+2, tlb_scroll+i);

		if(tlb_scroll > 0) triangle_up(_(7,38));
		if(tlb_scroll < TLB_VIEW_MAX) triangle_down(_(49,192));
	}

	#ifdef FX9860G
	extern bopti_image_t img_opt_gint_tlb;
	dimage(0, 56, &img_opt_gint_tlb);
	#endif

	#ifdef FXCG50
	fkey_menu(1, "ROM");
	fkey_menu(2, "INFO");
	fkey_menu(3, "TLB");
	fkey_action(5, "MISS");
	fkey_action(6, "TIMER");
	#endif
}

static int generate_tlb_miss(volatile void *arg)
{
	uint8_t volatile *next_miss = arg;
	GUNUSED uint8_t volatile x = *next_miss;
	return 1;
}

/* gintctl_gint_tlb(): TLB miss handler and TLB management */
void gintctl_gint_tlb(void)
{
	key_event_t ev;
	int key=0, tab=0;

	uint8_t pages[PAGE_COUNT];
	uint32_t next_miss = 0xffffffff;
	int tlb_scroll = 0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		if(tab == 0 || tab == 1)
			explore_pages(pages, &next_miss);
		draw(tab, pages, next_miss, tlb_scroll);

		dupdate();
		key = (ev = getkey()).key;

		if(key == KEY_F1) tab = 0;
		if(key == KEY_F2) tab = 1;
		if(key == KEY_F3) tab = 2;

		if(tab == 2 && key == KEY_UP)
		{
			if(ev.shift) tlb_scroll = 0;
			else if(tlb_scroll > 0) tlb_scroll--;
		}
		if(tab == 2 && key == KEY_DOWN)
		{
			if(ev.shift) tlb_scroll = TLB_VIEW_MAX;
			else if (tlb_scroll < TLB_VIEW_MAX) tlb_scroll++;
		}

		if(key == KEY_F5 && next_miss != 0xffffffff)
		{
			GUNUSED uint8_t volatile x;
			x = *(uint8_t volatile *)next_miss;
		}
		if(key == KEY_F6 && next_miss != 0xffffffff)
		{
			timer_setup(1, timer_delay(1, 10000), timer_default,
				generate_tlb_miss, (volatile void *)next_miss);
			timer_start(1);
			timer_wait(1);
		}
	}
}
