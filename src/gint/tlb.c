#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <gint/mmu.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

#define PAGE_USED    0x01
#define PAGE_MAPPED  0x02
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
		C_RGB(31,24,0),		/* Unused (data) but still mapped  */
		C_RGB(0,31,0),		/* Used and currently mapped */
	};
	uint16_t border =
		status == -1 ? C_RGB(0,0,31) :
		status == -2 ? C_RGB(31,0,0) :
		C_BLACK;

	uint16_t fill = (status >= 0 && status < 4) ? colors[status] : C_NONE;
	drect_border(x, y, x+SQUARE_WIDTH, y+SQUARE_HEIGHT, fill, 1, border);
}
#endif

#ifdef FX9860G
#define PAGE_COUNT 0x80
#define SQUARE_WIDTH 5
#define SQUARE_HEIGHT 5
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

static void explore_pages(uint8_t *pages, uint32_t *next_miss)
{
	extern uint32_t srom;

	for(uint p = 0; p < PAGE_COUNT; p++)
	{
		pages[p] = ((p << 12) < (uint32_t)&srom) ? PAGE_USED : 0;
	}

	if(isSH3())
	{
		for(int way = 0; way < 4; way++)
		for(int E = 0; E < 32; E++)
		{
			tlb_addr_t addr = *tlb_addr(way, E);
			tlb_data_t data = *tlb_data(way, E);
			if(!addr.V || !data.V || !data.SZ) continue;

			uint32_t src = (((addr.VPN >> 2) | E) << 12);
			int p = (src - 0x00300000) >> 12;
			pages[p] |= PAGE_MAPPED;
		}
	}
	else for(uint E = 0; E < 64; E++)
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
	#define fg_ fg, C_NONE, DTEXT_LEFT, DTEXT_TOP

	int y = row_y(row) + 2 * (row > 1);

	dprint_opt( 14, y, fg_, "%d", E);
	dprint_opt( 38, y, fg_, "%08X", src);
	dprint_opt(110, y, fg_, "%08X", dst);
	dprint_opt(182, y, fg_, "%s", size_str[size]);
	dprint_opt(222, y, fg_, "%s", access_str[data.PR]);
	dprint_opt(270, y, fg_, "%s", (data.SH ? "SH" : "-"));
	dprint_opt(294, y, fg_, "%s", (data.WT ? "WT" : "CB"));
	dprint_opt(318, y, fg_, "%s", (data.C ? "C" : "-"));
	dprint_opt(334, y, fg_, "%s", (addr.D || data.D ? "D" : "-"));
	dprint_opt(350, y, fg_, "%s", (addr.V && data.V ? "V" : "-"));
}
#endif

#ifdef FX9860G
void show_utlb(int row, int E)
{
	extern font_t font_mini;
	font_t const *old_font = dfont(&font_mini);
	int y = (row - 1) * 6 + 2 * (row > 1);

	if(E == -1)
	{
		dprint(  1, y, C_BLACK, "ID");
		dprint( 12, y, C_BLACK, "Virtual");
		dprint( 47, y, C_BLACK, "Physical");
		dprint( 82, y, C_BLACK, "Len");
		dprint( 97, y, C_BLACK, "Pr");
		dprint(115, y, C_BLACK, "CD");

		dfont(old_font);
		return;
	}

	char const *size_str[] = { "1k", "4k", "64k", "1M" };
	char const *access_str[] = { "Kr", "Krw", "Ur", "Urw" };
	char const *cd_str[] = { "--", "-D", "C-", "CD" };
	uint32_t src, dst;
	int valid, size, pr, cd;

	if(isSH3())
	{
		int way = (E >> 5);
		E &= 31;

		tlb_addr_t addr = *tlb_addr(way, E);
		tlb_data_t data = *tlb_data(way, E);

		valid = (addr.V != 0 && data.V != 0);
		size = data.SZ;

		if(data.SZ) src = (((addr.VPN >> 2) | E) << 12);
		else src = (addr.VPN | (E << 2)) << 10;

		dst = data.PPN << 10;
		pr = data.PR;
		cd = (data.C << 1) | data.D;
	}
	else
	{
		utlb_addr_t addr = *utlb_addr(E);
		utlb_data_t data = *utlb_data(E);

		src = addr.VPN << 10;
		dst = data.PPN << 10;

		valid = (addr.V != 0) && (data.V != 0);
		size = (data.SZ1 << 1) | data.SZ2;
		pr = data.PR;
		cd = (data.C << 1) | data.D;
	}

	dprint( 1, y, C_BLACK, "%d", E);

	if(valid)
	{
		dprint( 12, y, C_BLACK, "%08X", src);
		dprint( 47, y, C_BLACK, "%08X", dst);
		dprint( 82, y, C_BLACK, "%s", size_str[size]);
		dprint( 97, y, C_BLACK, "%s", access_str[pr]);
		dprint(115, y, C_BLACK, "%s", cd_str[cd]);
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

	#ifdef FX9860G
	extern font_t font_mini;
	font_t const *old_font = dfont(&font_mini);
	#endif

	if(tab == 0)
	{
		#ifdef FXCG50
		row_print(1, 1, "Size of ROM sections: %08X (%d pages of 4k)",
			rom_size, rom_pages);
		#endif

		for(uint p=0, y=_(12,36); p < PAGE_COUNT; p += 2*LINE_SIZE)
		{
			dprint(_(5,18), y, C_BLACK, "%08X",
				0x00300000 + (p << 12));
			y += 2*SQUARE_HEIGHT;
		}

		for(uint p = 0; p < PAGE_COUNT; p++)
		{
			draw_rom_cell(_(39,88) + rom_cell_x(p),
				_(11,36) + rom_cell_y(p), pages[p]);
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
		draw_rom_cell(_(8,18),   _(15,36), 0);
		draw_rom_cell(_(64,200), _(15,36), 1);
		draw_rom_cell(_(8,18),   _(22,50), 2);
		draw_rom_cell(_(64,200), _(22,50), 3);

		dprint(_(16,30),  _(15,36), C_BLACK,
			_("Unused", "Unused in add-in"));
		dprint(_(72,212), _(15,36), C_BLACK,
			_("Unmapped", "Currently unmapped"));
		dprint(_(16,30),  _(22,50), C_BLACK,
			_("Data", "Data still mapped"));
		dprint(_(72,212), _(22,50), C_BLACK,
			_("Mapped", "Currently mapped"));

		#ifdef FXCG50
		draw_rom_cell(18, 64, -1);
		dprint(30, 64, C_BLACK, "Next page to load");

		row_print(6, 1,
			"The LOAD key will load an unmapped page to TLB by");
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
		dprint(1, 30, C_BLACK, "Size of ROM text: %X (%d pages)",
			rom_size, rom_pages);

		if(next_miss != 0xffffffff)
			dprint(1, 36, C_BLACK, "Next load: %08x", next_miss);
		else
			dprint(1, 36, C_BLACK, "No page left to load!");

		dprint(1, 43, C_BLACK, "F5: Load a page by TLB miss");
		dprint(1, 49, C_BLACK, "F6: Do it from a timer callback");
		#endif
	}

	else if(tab == 2)
	{
		show_utlb(1, -1);
		for(int i = 0; i < TLB_VIEW; i++)
			show_utlb(i+2, tlb_scroll+i);

		#ifdef FX9860G
		dhline(6, C_BLACK);
		#endif

		#ifdef FXCG50
		dline(12, 34, 363, 34, C_BLACK);
		#endif

		scrollbar_px(
			/*    view */ _(8,37), _(54,201),
			/*   range */ 0, TLB_VIEW_MAX + TLB_VIEW,
			/* visible */ tlb_scroll, TLB_VIEW);
	}

	#ifdef FX9860G
	dfont(old_font);

	extern bopti_image_t img_opt_gint_tlb;
	dimage(0, 56, &img_opt_gint_tlb);
	#endif

	#ifdef FXCG50
	fkey_menu(1, "ROM");
	fkey_menu(2, "INFO");
	fkey_menu(3, "TLB");
	fkey_action(5, "LOAD");
	fkey_action(6, "TIMER");
	#endif
}

static int generate_tlb_miss(volatile void *arg)
{
	uint8_t volatile *next_miss = arg;
	GUNUSED uint8_t volatile x = *next_miss;
	return TIMER_STOP;
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
			if(ev.shift || keydown(KEY_SHIFT))
				tlb_scroll = 0;
			else if(tlb_scroll > 0)
				tlb_scroll--;
		}
		if(tab == 2 && key == KEY_DOWN)
		{
			if(ev.shift || keydown(KEY_SHIFT))
				tlb_scroll = TLB_VIEW_MAX;
			else if (tlb_scroll < TLB_VIEW_MAX)
				tlb_scroll++;
		}

		if(key == KEY_F5 && next_miss != 0xffffffff)
		{
			GUNUSED uint8_t volatile x;
			x = *(uint8_t volatile *)next_miss;
		}
		if(key == KEY_F6 && next_miss != 0xffffffff)
		{
			int timer = timer_setup(TIMER_ANY, 10000,
				generate_tlb_miss, next_miss);
			if(timer >= 0)
			{
				timer_start(timer);
				timer_wait(timer);
			}
		}
	}
}
