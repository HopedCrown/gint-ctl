#define GINT_NEED_VRAM

#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/syscalls.h>
#include <gint/gint.h>
#include <gint/hardware.h>

#include <gintctl/util.h>
#include <gintctl/menu.h>
#include <gintctl/prof-contexts.h>

#include <gintctl/gint.h>
#include <gintctl/perf.h>
#include <gintctl/mem.h>

#include <libprof.h>

/* Drawing functions:
   * ...
   Keyboard tests:
   * ...
   Timer tests:
   * Do more in-depth things than previous application
   * Stress testing for number of interrupts
   * ...
   TODO:
   * Add-in size and mappings
   * Interrupt controller state?
   * Clock frequencies
   * F2 to save hardware data to file */

/* gint test menu */
struct menu menu_gint = {
	_("gint tests", "gint features and driver tests"), .entries = {

	{ "Hardware",         gintctl_gint_hardware },
	{ "Boot log",         NULL },
	{ "Keyboard",         NULL },
	{ "Timers",           gintctl_gint_timer },
	{ "Real-time clock",  NULL },
	{ "Image rendering",  gintctl_gint_bopti },
	{ "Text rendering",   NULL },
	#ifdef FX9860G
	{ "Gray engine",      gintctl_gint_gray },
	{ "Gray rendering",   gintctl_gint_grayrender },
	#endif
	{ NULL, NULL },
}};

/* Performance menu */
struct menu menu_perf = {
	_("Performance", "Performance benchmarks"), .entries = {

	{ "libprof basics",      gintctl_perf_libprof },
	{ "Rendering functions", gintctl_perf_render },
	{ NULL, NULL },
}};

void exch_debug_thing(__attribute__((unused)) int code)
{
	#ifdef FXCG50

	uint32_t TEA = *((volatile uint32_t *)0xff00000c);
	uint32_t TRA = *((volatile uint32_t *)0xff000020);
	uint32_t PC;

	__asm__("stc spc, %0" : "=r"(PC));
	TRA = TRA >> 2;

	dclear(C_WHITE);

	print(6, 3, "An exception occured! (System ERROR)");

	uint32_t *long_vram = (void *)vram;
	for(int i = 0; i < 198 * 16; i++) long_vram[i] = ~long_vram[i];

	char const *name = "";
	if(code == 0x040) name = "TLB miss (nonexisting address) on read";
	if(code == 0x060) name = "TLB miss (nonexisting address) on write";
	if(code == 0x0e0) name = "Read address error (probably alignment)";
	if(code == 0x100) name = "Write address error (probably alignment)";
	if(code == 0x160) name = "Unconditional trap";
	if(code == 0x180) name = "Illegal instruction";
	if(code == 0x1a0) name = "Illegal delay slot instruction";

	print(6, 25, "%03x %s", code, name);

	print(6, 45, "PC");
	print(38, 45, "= %08x", PC);
	print(261, 45, "(Error location)");

	print(6, 60, "TEA");
	print(38, 60, "= %08x", TEA);
	print(234, 60, "(Offending address)");

	print(6, 75, "TRA");
	print(38, 75, "= %#x", TRA);
	print(281, 75, "(Trap number)");

	print(6, 95,  "An unrecoverable error ocurred in the add-in.");
	print(6, 108, "Please press the RESET button to restart the");
	print(6, 121, "calculator.");

	dupdate_noint();
	#endif

	while(1);
}

//---
//	Main application
//---

/* gintctl_main(): Show the main tab */
void gintctl_main(void)
{
	#ifdef FX9860G
	row_title("gint @%07x", GINT_VERSION);

	row_print(3, 1, "F2:gint tests");
	row_print(4, 1, "F3:Performance");
	row_print(5, 1, "F5:MPU registers");
	row_print(6, 1, "F6:Memory map/dump");
	#endif /* FX9860G */

	#ifdef FXCG50
	row_title("gint @%07x for fx-CG 50", GINT_VERSION);
	row_print(1, 1, "F2: gint features and driver tests");
	row_print(2, 1, "F3: Performance benchmarks");
	row_print(3, 1, "F5: MPU register browser");
	row_print(4, 1, "F6: Hexadecimal memory editor");

	#ifdef GINT_BOOTLOG
	extern char gint_bootlog[22 * 8];
	extern font_t *gint_default_font;

	for(int i = 1; i < 8; i++) dtext(8, 85 + 13 * i, gint_bootlog + 22 * i,
		C_BLACK, C_NONE);
	#endif /* GINT_BOOTLOG */
	#endif /* FXCG50 */
}

int main(GUNUSED int isappli, GUNUSED int optnum)
{
	/* Initialize menu metadata */
	int top = _(1, 0), bottom = _(1, 0);
	menu_init(&menu_gint, top, bottom);
	menu_init(&menu_perf, top, bottom);

	/* Start the profiling library */
	prof_init(PROFCTX_COUNT, 2);

	int key = 0;
	struct menu *menu = NULL;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		if(menu) menu_show(menu);
		else gintctl_main();

		#ifdef FX9860G
		extern image_t img_opt_main;
		dimage(0, 56, &img_opt_main);
		#else
		fkey_action(1, "INFO");
		fkey_menu(2, "GINT");
		fkey_menu(3, "PERF");
		fkey_button(5, "REGS");
		fkey_button(6, "MEMORY");
		#endif

		dupdate();
		key = getkey().key;

		if(key == KEY_F1)
			menu = NULL;
		if(key == KEY_F2)
			menu = &menu_gint;
		if(key == KEY_F3)
			menu = &menu_perf;
		if(key == KEY_F5)
			gintctl_regs();
		if(key == KEY_F6)
			gintctl_mem();

		if(!menu) continue;

		if(key == KEY_UP || key == KEY_DOWN)
			menu_move(menu, key, 0);
		if(key == KEY_EXE)
			menu_exec(menu);
	}

	prof_quit();
	return 0;
}
