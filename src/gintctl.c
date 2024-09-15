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
#include <gintctl/assets.h>

#include <gintctl/gint.h>
#include <gintctl/perf.h>
#include <gintctl/mem.h>
#include <gintctl/ui.h>

#include <libprof.h>

#include <fxlibc/printf.h>

#if GINT_HW_CP
GSECTION(".hh2.info") GVISIBLE
char _hh2info[] = "GINTCTL\0gint control application\0Lephe\0" "2.10";
#endif

/* TODO:
   * Interrupt controller state?
   * Clock frequencies
   * F2 to save hardware data to file */

#if GINT_RENDER_MONO
# define CTGY(SH, LG)
#else
# define CTGY(SH, LG) {LG, NULL, MENU_CATEGORY},
#endif

/* gint test menu */
struct menuentry menu_gint[] = {
	CTGY("gint tests", "gint features and driver tests")
	{ "CPU and memory",     gintctl_gint_cpumem, 0 },
	{ "RAM discovery",      gintctl_gint_ram, MENU_SH4_ONLY },
	#if !GINT_HW_CP
	{ "Memory dump",        gintctl_gint_dump, 0 },
	#endif
	{ "Drivers and worlds", gintctl_gint_drivers, 0 },
	#if !GINT_HW_CP
	{ "BFile filesystem",   gintctl_gint_bfile, 0 },
	#endif
	{ "TLB management",     gintctl_gint_tlb, 0 },
	#if (GINT_HW_CG || GINT_HW_CP) && GINT_RENDER_RGB
	{ "Overclocking",       gintctl_gint_overclock, MENU_SH4_ONLY },
	#endif
	{ "Memory allocation",  gintctl_gint_kmalloc, 0 },
	{ "Syscall emulation",  gintctl_gint_syscallemu, 0 },
	{ "Keyboard",           gintctl_gint_keyboard, 0 },
	{ "Timers",             gintctl_gint_timer, 0 },
	{ "Timer callbacks",    gintctl_gint_timer_callbacks, 0 },
	{ "DMA control",        gintctl_gint_dma, MENU_SH4_ONLY },
	{ "Real-time clock",    gintctl_gint_rtc, 0 },
	#if !GINT_HW_CP
	{ "USB communication",  gintctl_gint_usb, MENU_SH4_ONLY },
	#endif
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
	#if !GINT_HW_CP
	{ "GDB",                gintctl_gint_gdb, MENU_SH4_ONLY },
	#endif
	{ NULL, NULL, 0 },
};

/* Performance menu */
struct menuentry menu_perf[] = {
	CTGY("Performance", "Performance benchmarks")
	{ "libprof basics",      gintctl_perf_libprof, 0 },
#if !GINT_HW_CP
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
#endif
	{ NULL, NULL, 0 },
};

//---
// Global shortcuts
//---

#if !GINT_HW_CP

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

// TODO: Replace gintctl_getkey and related mechanisms with JustUI events
// Note: this requires a single scene for the entire application
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
#else
#define gintctl_getkey getkey
#endif

//---
// UI/scene management: a global JustUI scene with a stack of gscreens in it
//---

static jscene *scene = NULL;

jscene *gintctl_scene(void)
{
	return scene;
}

void gintctl_scene_init(void)
{
	scene = jscene_create_fullscreen(NULL);
	jlayout_set_stack(scene);
	jwidget_set_background(scene, C_WHITE);
	jscene_set_autopaint(scene, true);
}

gscreen *gintctl_scene_push(gscreen *s)
{
	if(!s)
		return NULL;
	jwidget_add_child(scene, s);
	jlayout_get_stack(scene)->active = scene->widget.child_count - 1;
	scene->widget.update = 1;
	jwidget_scope_set_target(scene, s);
	return s;
}

void gintctl_scene_pop(void)
{
	/* Don't pop the last screen */
	int N = scene->widget.child_count;
	if(N >= 2) {
		jwidget *child = scene->widget.children[N - 1];
		jwidget_remove_child(scene, child);
		jwidget_destroy(child);
		jlayout_get_stack(scene)->active = N - 2;
		jwidget_scope_set_target(scene, scene->widget.children[N - 2]);
	}
	scene->widget.update = 1;
}

void gintctl_scene_deinit(void)
{
	jwidget_destroy(scene);
}

//---
//	Main application
//---

int main(void)
{
	gint_setrestart(1);

	/* Enable global getkey() shortcuts */
#if !GINT_HW_CP
	getkey_set_feature_function(getkey_global_shortcuts);
#endif

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
#if !GINT_HW_CP
	usb_fxlink_set_notifier(gintctl_fxlink_notification);
#endif

	/* Enable keyboard options globally because we're going to interrupt
	   getkey_opt() to answer USB requests synchronously */
	keydev_transform_t tr = keydev_transform(keydev_std());
	tr.enabled |= KEYDEV_TR_DELAYED_SHIFT | KEYDEV_TR_INSTANT_SHIFT;
	tr.enabled |= KEYDEV_TR_DELAYED_ALPHA | KEYDEV_TR_INSTANT_ALPHA;
	keydev_set_transform(keydev_std(), tr);

	gintctl_scene_init();

	//---
	// Main menu UI
	//---

	gscreen *s = gscreen_create2("", &img_opt_main, "",
		"/INFO;/GINT;/PERF;;@REGS;@MEMORY", NULL);
	gintctl_scene_push(s);

	// TODO: Better macro distinctions
	jlabel_asprintf(s->title,
#if GINT_HW_CG
		"gint %s (@%07x) for fx-CG", GINT_VERSION, GINT_HASH
#elif GINT_HW_CP
		"gint %s (@%07x) for fx-CP", GINT_VERSION, GINT_HASH
#else
		"gint %s %07x", GINT_VERSION, GINT_HASH
#endif
	);

	char const *main_menu_str = _(
		"F2:gint tests\n"
		"F3:Performance\n"
		"F5:MPU registers\n"
		"F6:Memory map/dump",
		//---
		"F2: gint features and driver tests\n"
		"F3: Performance benchmarks\n"
		"F5: MPU register browser (WIP)\n"
		"F6: Hexadecimal memory browser\n"
		"\n"
		"This add-in is running a unikernel called gint by Lephe'. "
		"Information about the project is available on planet-casio.com.");

	jlabel *main_menu_label = jlabel_create(main_menu_str, NULL);
	jlabel_set_wrap_mode(main_menu_label, J_WRAP_WORD);
	jwidget_set_padding(main_menu_label, _(1,3), _(1,3), _(1,3), _(1,3));
	gscreen_add_tab(s, main_menu_label, NULL);

	gmenu *menu1 = gmenu_create(menu_gint, NULL);
	gscreen_add_tab(s, menu1, menu1->list);

	gmenu *menu2 = gmenu_create(menu_perf, NULL);
	gscreen_add_tab(s, menu2, menu2->list);

	while(true) {
		jevent e = jscene_run(gintctl_scene());

		if(e.type == JLIST_ITEM_TRIGGERED) {
			struct menuentry const *entries = ((jlist *)e.source)->user;
			entries[e.data].function();
			gintctl_scene_pop();
		}

		if(jevent_is_press(e, KEY_EXIT))
			break;

		if(e.type == JFKEYS_TRIGGERED && e.data == 0)
			gscreen_show_tab(s, 0);
		if(e.type == JFKEYS_TRIGGERED && e.data == 1)
			gscreen_show_tab(s, 1);
		if(e.type == JFKEYS_TRIGGERED && e.data == 2)
			gscreen_show_tab(s, 2);
		if(e.type == JFKEYS_TRIGGERED && e.data == 4) {
			gintctl_regs();
			gintctl_scene_pop();
		}
		if(e.type == JFKEYS_TRIGGERED && e.data == 5) {
			gintctl_mem();
			gintctl_scene_pop();
		}
	}

	/* Prepare a main menu frame to maintain the illusion when coming
	   back after a restart */
	gscreen_show_tab(s, 0);
	jscene_render(scene);

	gintctl_scene_deinit();
	prof_quit();
	return 0;
}
