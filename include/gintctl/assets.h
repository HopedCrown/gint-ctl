//---
//	gintctl:assets - List of imported assets
//---

#ifndef _GINTCTL_ASSETS
#define _GINTCTL_ASSETS

#include <gint/config.h>

#if GINT_RENDER_MONO

#include <libimg.h>

extern font_t
	font_hexa,
	font_mini,
	font_title,
	font_uf5x7;

extern bopti_image_t
	img_bopti_1col,
	img_bopti_2col,
	img_bopti_3col,
	img_kbd_events,
	img_kbd_pressed,
	img_kbd_released,
	img_opt_dump,
	img_opt_gint_bopti,
	img_opt_gint_cpumem,
	img_opt_gint_drivers,
	img_opt_gint_gdb,
	img_opt_gint_gray,
	img_opt_gint_keyboard,
	img_opt_gint_kmalloc,
	img_opt_gint_ram,
	img_opt_gint_rtc,
	img_opt_gint_spuram,
	img_opt_gint_timer_callbacks,
	img_opt_gint_timers,
	img_opt_gint_tlb,
	img_opt_gint_usb,
	img_opt_gint_bfile,
	img_opt_main,
	img_opt_mem,
	img_opt_perf_cpu,
	img_opt_perf_libprof,
	img_opt_perf_memory,
	img_opt_perf_memory_sh3,
	img_opt_perf_render,
	img_profile_gray_alpha,
	img_profile_gray,
	img_profile_mono_alpha,
	img_profile_mono,
	img_rtc_arrows,
	img_rtc_segments,
	img_tlb_cells;

extern img_t
	img_libimg_swords;

#endif

#if GINT_RENDER_RGB

extern bopti_image_t
	img_kbd_events,
	img_kbd_pressed,
	img_kbd_released,
	img_rtc_arrows,
	img_rtc_segments,
	img_swift,
	img_swords,
	img_libimg_even_odd,
	img_libimg_odd_even,
	img_libimg_sq_even,
	img_libimg_sq_odd,
	img_libimg_train;

#endif

#endif /* _GINTCTL_ASSETS */
