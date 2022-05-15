#define GINT_NEED_VRAM

#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/gint.h>
#include <gint/drivers/keydev.h>
#include <gint/hardware.h>
#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>

#ifdef FX9860G
#include <gint/gray.h>
#endif

#include <gintctl/util.h>
#include <gintctl/menu.h>
#include <gintctl/assets.h>

#include <gintctl/gint.h>
#include <gintctl/perf.h>
#include <gintctl/libs.h>
#include <gintctl/mem.h>

#include <libprof.h>

#include <fxlibc/printf.h>

/* TODO:
   * Interrupt controller state?
   * Clock frequencies
   * F2 to save hardware data to file */

/* gint test menu */
struct menu menu_gint = {
	_("gint tests", "gint features and driver tests"), .entries = {

	{ "CPU and memory",     gintctl_gint_cpumem, 0 },
	{ "RAM discovery",      gintctl_gint_ram, MENU_SH4_ONLY },
	#ifdef FXCG50
	{ "DSP processors",     gintctl_gint_dsp, 0 },
	#endif
	{ "SPU memory",         gintctl_gint_spuram, MENU_SH4_ONLY },
	{ "Memory dump",        gintctl_gint_dump, 0 },
	{ "Drivers and worlds", gintctl_gint_drivers, 0 },
	{ "TLB management",     gintctl_gint_tlb, 0 },
	#ifdef FXCG50
	{ "Overclocking",       gintctl_gint_overclock, MENU_SH4_ONLY },
	#endif
	{ "Memory allocation",  gintctl_gint_kmalloc, 0 },
	{ "Keyboard",           gintctl_gint_keyboard, 0 },
	{ "Timers",             gintctl_gint_timer, 0 },
	{ "Timer callbacks",    gintctl_gint_timer_callbacks, 0 },
	{ "DMA control",        gintctl_gint_dma, MENU_SH4_ONLY },
	{ "Real-time clock",    gintctl_gint_rtc, 0 },
	{ "USB communication",  gintctl_gint_usb, MENU_SH4_ONLY },
	{ "Image rendering",    gintctl_gint_image, 0 },
	{ "Text rendering",     gintctl_gint_topti, 0 },
	#ifdef FX9860G
	{ "Gray engine",        gintctl_gint_gray, 0 },
	{ "Gray rendering",     gintctl_gint_grayrender, 0 },
	#endif
	{ NULL, NULL, 0 },
}};

/* Performance menu */
struct menu menu_perf = {
	_("Performance", "Performance benchmarks"), .entries = {

	{ "libprof basics",      gintctl_perf_libprof, 0 },
	{ "CPU and cache",       gintctl_perf_cpucache, 0 },
	{ _("CPU parallelism", "Superscalar and pipeline parallelism"),
	                         gintctl_perf_cpu, 0 },
	{ "Interrupt stress",    gintctl_perf_interrupts, 0 },
	#ifdef FXCG50
	{ "Memory read/write speed",
	                         gintctl_perf_memory, 0 },
	#endif
	{ "Rendering functions", gintctl_perf_render, 0 },

	/* TODO: Comparison with MonochromeLib */

	{ NULL, NULL, 0 },
}};

/* External libraries */
struct menu menu_libs = {
	_("Libraries", "External and standard libraries"), .entries = {

	{ "libm: " _("OpenLibm", "OpenLibm floating-point functions"),
		gintctl_libs_openlibm, 0 },
	{ "JustUI widgets",
		gintctl_libs_justui, 0 },
	{ "BFile filesystem",
		gintctl_libs_bfile, 0 },
	{ NULL, NULL, 0 },
}};

//---
// Global shortcuts
//---

/* Whether we're recording */
static bool getkey_recording = false;

static void getkey_record_video_frame(int onscreen)
{
		#ifdef FX9860G
		if(dgray_enabled())
			usb_fxlink_videocapture_gray(true);
		else
			usb_fxlink_videocapture(onscreen);
		#endif

		#ifdef FXCG50
		usb_fxlink_videocapture(onscreen);
		#endif
}

static bool getkey_global_shortcuts(key_event_t e)
{
	if(usb_is_open() && e.key == KEY_OPTN && !e.shift && !e.alpha) {
		#ifdef FX9860G
		if(dgray_enabled())
			usb_fxlink_screenshot_gray(true);
		else
			usb_fxlink_screenshot(true);
		#endif

		#ifdef FXCG50
		usb_fxlink_screenshot(true);
		#endif

		return true;
	}
	if(usb_is_open() && e.key == KEY_VARS && e.shift && !e.alpha) {
		if(!getkey_recording) {
			dupdate_set_hook(GINT_CALL(getkey_record_video_frame, (int)false));
			getkey_record_video_frame(true);
			getkey_recording = true;
		}
		else {
			dupdate_set_hook(GINT_CALL_NULL);
			getkey_recording = false;
		}
	}
	return false;
}

//---
//	Main application
//---

/* gintctl_main(): Show the main tab */
void gintctl_main(void)
{
	#ifdef FX9860G
	row_title("gint %s %07x", GINT_VERSION, GINT_HASH);

	row_print(3, 1, "F2:gint tests");
	row_print(4, 1, "F3:Performance");
	row_print(5, 1, "F4:Libraries");
	row_print(6, 1, "F5:MPU registers");
	row_print(7, 1, "F6:Memory map/dump");
	#endif /* FX9860G */

	#ifdef FXCG50
	row_title("gint %s (@%07x) for fx-CG 50", GINT_VERSION, GINT_HASH);
	row_print(1,1, "F2: gint features and driver tests");
	row_print(2,1, "F3: Performance benchmarks");
	row_print(3,1, "F4: External libraries");
	row_print(4,1, "F5: MPU register browser");
	row_print(5,1, "F6: Hexadecimal memory browser");

	row_print(7,1, "This add-in is running a unikernel called gint by");
	row_print(8,1, "Lephe'. Information about the project is available");
	row_print(9,1, "on planet-casio.com.");
	#endif /* FXCG50 */
}

static void draw(struct menu *menu)
{
	dclear(C_WHITE);

	if(menu) menu_show(menu);
	else gintctl_main();

	#ifdef FX9860G
	dimage(0, 56, &img_opt_main);
	#endif

	#ifdef FXCG50
	fkey_action(1, "INFO");
	fkey_menu(2, "GINT");
	fkey_menu(3, "PERF");
	fkey_menu(4, "LIBS");
	fkey_button(5, "REGS");
	fkey_button(6, "MEMORY");
	#endif
}

int main(GUNUSED int isappli, GUNUSED int optnum)
{
	/* Initialize menu metadata */
	int top = _(1, 0), bottom = 1;
	menu_init(&menu_gint, top, bottom);
	menu_init(&menu_perf, top, bottom);
	menu_init(&menu_libs, top, bottom);

	#ifdef FX9860G
	gint_setrestart(1);
	#endif

	/* Enable global getkey() shortcuts */
	getkey_set_feature_function(getkey_global_shortcuts);

	/* Start the profiling library */
	prof_init();

	/* Enable floating-point formatters */
	__printf_enable_fp();
	/* Enable fixed-point formatters */
	__printf_enable_fixed();

	#ifdef FX9860G
	/* Use the Unicode font uf5x7 on fx-9860G */
	dfont(&font_uf5x7);
	#endif

	keydev_transform_t tr = keydev_transform(keydev_std());
	tr.enabled |= KEYDEV_TR_DELAYED_SHIFT | KEYDEV_TR_INSTANT_SHIFT;
	tr.enabled |= KEYDEV_TR_DELAYED_ALPHA | KEYDEV_TR_INSTANT_ALPHA;
	keydev_set_transform(keydev_std(), tr);

	key_event_t ev;
	int key = 0;
	struct menu *menu = NULL;

	while(key != KEY_EXIT)
	{
		draw(menu);
		dupdate();

		ev = getkey();
		key = ev.key;

		if(key == KEY_F1)
			menu = NULL;
		if(key == KEY_F2)
			menu = &menu_gint;
		if(key == KEY_F3)
			menu = &menu_perf;
		if(key == KEY_F4)
			menu = &menu_libs;
		if(key == KEY_F5)
			gintctl_regs();
		if(key == KEY_F6)
			gintctl_mem();

		if(!menu) continue;

		if(key == KEY_UP || key == KEY_DOWN)
			menu_move(menu, key, ev.shift || keydown(KEY_SHIFT),0);
		if(key == KEY_EXE)
			menu_exec(menu);
	}

	/* Prepare a main menu frame to maintain the illusion when coming
	   back after a restart */
	draw(NULL);

	prof_quit();
	return 0;
}
