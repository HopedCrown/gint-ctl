#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/gint.h>
#include <gint/drivers/keydev.h>
#include <gint/hardware.h>
#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>
#include <gint/config.h>
#include <gint/gray.h>

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
	#if GINT_HW_CG && GINT_RENDER_RGB
	{ "DSP processors",     gintctl_gint_dsp, 0 },
	#endif
	{ "SPU memory",         gintctl_gint_spuram, MENU_SH4_ONLY },
	{ "Memory dump",        gintctl_gint_dump, 0 },
	{ "Drivers and worlds", gintctl_gint_drivers, 0 },
	{ "TLB management",     gintctl_gint_tlb, 0 },
	#if GINT_HW_CG && GINT_RENDER_RGB
	{ "Overclocking",       gintctl_gint_overclock, MENU_SH4_ONLY },
	#endif
	{ "Memory allocation",  gintctl_gint_kmalloc, 0 },
	{ "Keyboard",           gintctl_gint_keyboard, 0 },
	{ "Timers",             gintctl_gint_timer, 0 },
	{ "Timer callbacks",    gintctl_gint_timer_callbacks, 0 },
	{ "DMA control",        gintctl_gint_dma, MENU_SH4_ONLY },
	{ "Real-time clock",    gintctl_gint_rtc, 0 },
	{ "USB communication",  gintctl_gint_usb, MENU_SH4_ONLY },
	#if GINT_HW_CG && GINT_RENDER_RGB
	{ "USB tracer",         gintctl_gint_usbtrace, MENU_SH4_ONLY },
	#endif
	{ "Basic rendering",    gintctl_gint_render, 0 },
	{ "Image rendering",    gintctl_gint_image, 0 },
	{ "Text rendering",     gintctl_gint_topti, 0 },
	#if GINT_HW_FX
	{ "Gray engine",        gintctl_gint_gray, 0 },
	#endif
	#if GINT_RENDER_MONO
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
	#if GINT_RENDER_RGB
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
	/* Auto stop when closing USB connection */
	if(!usb_is_open()) {
		dupdate_set_hook(GINT_CALL_NULL);
		return;
	}

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
	if(e.shift && e.key == KEY_COMMA) {
		static int stage = 0;
		stage = (stage + 1) % 8;
		int border = stage * _(3, 13);
		struct dwindow win = {
			.left = border,
			.top = border,
			.right = DWIDTH - border,
			.bottom = DHEIGHT - border,
		};
		dwindow_set((struct dwindow){ 0, 0, DWIDTH, DHEIGHT });
		dclear(_(C_WHITE, 0x5555));
		dwindow_set(win);
	}
	return false;
}

int volatile gintctl_interrupt = 0;

static void gintctl_fxlink_notification(void)
{
	/* Hack: use bit #31 to indicate an internal interrupt */
	gintctl_interrupt |= (1 << 31);
}

key_event_t gintctl_getkey_opt(int options)
{
	usb_fxlink_header_t header;

	while(1) {
		key_event_t ev = getkey_opt(options, &gintctl_interrupt);
		while(usb_fxlink_handle_messages(&header))
			gintctl_handle_usb_command(&header);

		/* Keep waiting only if we were interrupted *and* the interrupt only
		   set bit #31 */
		if(ev.type != KEYEV_NONE || ((gintctl_interrupt << 1) != 0)) {
			gintctl_interrupt = 0;
			return ev;
		}
	}
}

key_event_t gintctl_getkey(void)
{
	return gintctl_getkey_opt(GETKEY_DEFAULT);
}

//---
//	Main application
//---

/* gintctl_main(): Show the main tab */
void gintctl_main(void)
{
	#if GINT_RENDER_MONO
	row_title("gint %s %07x", GINT_VERSION, GINT_HASH);

	row_print(3, 1, "F2:gint tests");
	row_print(4, 1, "F3:Performance");
	row_print(5, 1, "F4:Libraries");
	row_print(6, 1, "F5:MPU registers");
	row_print(7, 1, "F6:Memory map/dump");
	#endif

	#if GINT_RENDER_RGB
	row_title("gint %s (@%07x) for fx-CG 50", GINT_VERSION, GINT_HASH);
	row_print(1,1, "F2: gint features and driver tests");
	row_print(2,1, "F3: Performance benchmarks");
	row_print(3,1, "F4: External libraries");
	row_print(4,1, "F5: MPU register browser");
	row_print(5,1, "F6: Hexadecimal memory browser");

	row_print(7,1, "This add-in is running a unikernel called gint by");
	row_print(8,1, "Lephe'. Information about the project is available");
	row_print(9,1, "on planet-casio.com.");
	#endif
}

static void draw(struct menu *menu)
{
	dclear(C_WHITE);

	if(menu) menu_show(menu);
	else gintctl_main();

	#if GINT_RENDER_MONO
	dimage(0, 56, &img_opt_main);
	#endif

	#if GINT_RENDER_RGB
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

	gint_setrestart(1);

	/* Enable global getkey() shortcuts */
	getkey_set_feature_function(getkey_global_shortcuts);

	/* Start the profiling library */
	prof_init();

	/* Enable floating-point formatters */
	__printf_enable_fp();
	/* Enable fixed-point formatters */
	__printf_enable_fixed();

	#if GINT_RENDER_MONO
	/* Use the Unicode font uf5x7 on fx-9860G */
	dfont(&font_uf5x7);
	#endif

	/* Get notified when fxlink messages arrive through USB */
	usb_fxlink_set_notifier(gintctl_fxlink_notification);

	/* Enable keyboard options globally because we're going to interrupt
	   getkey_opt() to answer USB requests synchronously */
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

		ev = gintctl_getkey();
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
