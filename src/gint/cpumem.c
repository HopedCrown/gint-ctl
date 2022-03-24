#include <gint/hardware.h>
#include <gint/keyboard.h>
#include <gint/display.h>
#include <gint/mmu.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

#include <stdio.h>

/* TODO: Include <gint/cpu.h> */
extern uint32_t cpu_getVBR(void);

/* Some symbols from the linker script */
extern uint32_t
	brom, srom,			/* Limits of ROM mappings */
	sdata,  rdata,			/* User's data section */
	sbss, rbss;			/* User's BSS section */
#ifdef FX9860G
extern uint32_t sgmapped;		/* Permanently mapped functions */
#endif


/* MPU type and processor version */
void show_mpucpu(void)
{
	char const *mpu_names[] = {
		#ifdef FX9860G
		"Unknown",
		"SH-3 SH7337",
		"SH-4A SH7305",
		"SH-3 SH7355",
		"SH-4A SH7724",
		#else
		"Unknown MPU product",
		"SH-3-based SH7337",
		"SH-4A-based SH7305",
		"SH-3-based SH7355",
		"SH-4A-based SH7724",
		#endif
	};
	char const *calc_names[] = {
		"Unknown",
		"fx-9860G-like",
		"fx-9860G-like",
		"Graph 35+E II",
		"Prizm fx-CG 20",
		"fx-CG 50/Graph 90+E",
		"fx-CG Manager",
	};
	char const *fs_names[] = {
		"Unknown",
		"CASIOWIN",
		"Fugue",
	};

	int mpu  = gint[HWMPU];
	int calc = gint[HWCALC];
	int fs   = gint[HWFS];

	extern volatile int cpu_sleep_block_counter;

	/* Generate a default calc name if invalid values are found */
	char calc_default[16];
	sprintf(calc_default, "<CALCID %d>", calc);
	char const *str_calc = calc_default;
	if(calc >= 0 && calc < 7) str_calc = calc_names[calc];

	/* Generate a default MPU name if invalid values are found */
	char mpu_default[16];
	sprintf(mpu_default, "<MPUID %d>", mpu);
	char const *str_mpu = mpu_default;
	if(mpu >= 0 && mpu < 5) str_mpu = mpu_names[mpu];

	/* Generate a default fs name if invalid values are found */
	char fs_default[16];
	sprintf(fs_default, "<FSID %d>", fs);
	char const *str_fs = fs_default;
	if(fs >= 0 && fs < 3) str_fs = fs_names[fs];

	volatile uint32_t *CPUOPM = (void *)0xff2f0000;
	uint32_t SR, r15;
	__asm__("stc sr,  %0" : "=r"(SR));
	__asm__("mov r15, %0" : "=r"(r15));

	#ifdef FX9860G
	extern font_t font_mini;
	font_t const *old_font = dfont(&font_mini);

	dprint(1, 10, C_BLACK, "Model: %s", str_calc);
	dprint(1, 16, C_BLACK, "MPU: %s", str_mpu);
	dprint(1, 22, C_BLACK, "Filesystem: %s", str_fs);

	print_prefix(29, 30,     "SR", "%08X", SR);
	dline(29, 36, 29, 52, C_BLACK);
	if(isSH3()) {
		print_prefix(29, 36,    "PVR", "");
		print_prefix(29, 42,    "PRR", "");
		print_prefix(29, 48, "CPUOPM", "");
	}
	else {
		print_prefix(29, 36,    "PVR", "%08X", gint[HWCPUVR]);
		print_prefix(29, 42,    "PRR", "%08X", gint[HWCPUPR]);
		print_prefix(29, 48, "CPUOPM", "%08X", *CPUOPM);
	}
	print_prefix(85, 30,    "VBR", "%08X", cpu_getVBR());
	print_prefix(85, 36,    "R15", "%08X", r15);
	print_prefix(85, 42,    "sbc", "%d", cpu_sleep_block_counter);
	dline(85, 30, 85, 46, C_BLACK);
	dfont(old_font);
	#endif

	#ifdef FXCG50
	row_print(1, 1, "Calculator model: %s", str_calc);
	row_print(3, 1, "MPU: %s", str_mpu);
	row_print(4, 1, " Status Register: %08x", SR);
	row_print(5, 1, " Processor Version Register: %08x", gint[HWCPUVR]);
	row_print(6, 1, " Product Register: %08x", gint[HWCPUPR]);
	row_print(7, 1, " CPU Operation Mode: %08x", *CPUOPM);
	row_print(8, 1, " Current VBR: %08x", cpu_getVBR());
	row_print(9, 1, " Current stack pointer: %08x", r15);
	row_print(10, 1, " CPU sleep block level: %d", cpu_sleep_block_counter);
	row_print(12, 1, "Filesystem type: %s", str_fs);
	#endif
}

/* Memory */
static void show_memory(void)
{
	#ifdef FX9860G
	extern font_t font_mini;
	font_t const *old_font = dfont(&font_mini);
	print_prefix(28, 10,   "brom", "%08X", &brom);
	print_prefix(28, 16,  "rdata", "%08X", &rdata);
	print_prefix(28, 22,   "rbss", "%08X", &rbss);
	print_prefix(28, 28, "rreloc", "%08X", mmu_uram());

	print_prefix(98, 10,   "srom", "%06d", &srom);
	print_prefix(98, 16,  "sdata", "%06d", &sdata);
	print_prefix(98, 22,   "sbss", "%06d", &sbss);
	print_prefix(98, 28, "sreloc", "%06d", &sgmapped);

	dprint(1, 38, C_BLACK, "ROM: %dk, RAM: %dk",
		gint[HWROM] >> 10, gint[HWRAM] >> 10);
	dprint(1, 45, C_BLACK, "User RAM: %08X (%dk, P0 %dk)",
		mmu_uram(), mmu_uram_size() >> 10, gint[HWURAM] >> 10);
	dfont(old_font);
	#endif

	#ifdef FXCG50
	uint32_t base_ram  = 0x88000000;
	if(gint[HWCALC] == HWCALC_FXCG50) base_ram = 0x8c000000;

	row_print(1, 1, "RAM: %dM, RAM: %dM (starts at %08X)",
		gint[HWROM] >> 20, gint[HWRAM] >> 20, base_ram);
	row_print(2, 1, "Userspace RAM: %08X (%dk continuous block)",
		mmu_uram(), mmu_uram_size() >> 10);
	row_print(3, 1, "Total RAM mapped in P0: %dk",
		gint[HWURAM] >> 10);

	print_prefix(80, row_y(5),   "brom", "%08X", &brom);
	print_prefix(80, row_y(6),  "rdata", "%08X", &rdata);
	print_prefix(80, row_y(7),   "rbss", "%08X", &rbss);

	print_prefix(240, row_y(5),   "srom", "%06d", &srom);
	print_prefix(240, row_y(6),  "sdata", "%06d", &sdata);
	print_prefix(240, row_y(7),   "sbss", "%06d", &sbss);
	#endif
}

#if 0
static void hw_keyboard(int *row)
{
	int kbd = gint[HWKBD];

	if(kbd & HWKBD_IO)
	{
		put(_(" I/O driven","Driven by I/O port scanning"));
		put(kbd & HWKBD_WDD
			? _(" Watchdog delay"," Watchdog timer I/O delays")
			: _(" Active delay"," Active-waiting I/O delays"));
	}
	if(kbd & HWKBD_KSI)
	{
		put(_(" Key scan interface",
			" Driven by SH7305-style key scan interface"));
	}

	// + scan frequency
}

static void hw_display(int *row)
{
	int dd = gint[HWDD];

	#ifdef FXCG50
	if(dd & HWDD_KNOWN) put(" Known R61524-type model");
	if(dd & HWDD_FULL) put(" Fullscreen mode enabled (no borders)");
	#endif

	#ifdef FX9860G
	if(dd & HWDD_CONTRAST) put(" Contrast known");
	#endif

	if(dd & HWDD_LIGHT) put(_(" Backlight supported",
		" Backlight configuration is enabled"));
}
#endif

/* gintctl_gint_cpumem(): Detected CPU and memory configuration */
void gintctl_gint_cpumem(void)
{
	int tab=0, key=0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		if(tab == 0) show_mpucpu();
		if(tab == 1) show_memory();

		#ifdef FX9860G
		row_title("CPU and memory");
		extern bopti_image_t img_opt_gint_cpumem;
		dimage(0, 56, &img_opt_gint_cpumem);
		#endif

		#ifdef FXCG50
		row_title("Processor and memory");
		fkey_menu(1, "MPU/CPU");
		fkey_menu(2, "MEMORY");
		#endif

		dupdate();
		key = getkey().key;
		if(key == KEY_F1) tab = 0;
		if(key == KEY_F2) tab = 1;
	}
}
