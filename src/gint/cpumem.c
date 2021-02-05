#include <gint/hardware.h>
#include <gint/keyboard.h>
#include <gint/display.h>
#include <gint/mmu.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

#define put(...) row_print(++(*row), 1, __VA_ARGS__)

#define load_barrier(x)	{		\
	if(!((x) & HW_LOADED)) {	\
		put(" (not loaded)");	\
		return;			\
	}				\
}

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
		"SH3 fx-9860G*",
		"SH4 fx-9860G*",
		"Graph 35+E II",
		"Prizm fx-CG 20",
		"fx-CG 50/Graph 90+E",
		"fx-CG Manager",
	};

	int mpu  = gint[HWMPU];
	int calc = gint[HWCALC];

	char const *str_calc = _("Model", "Calculator model");
	if(calc < 0 || calc > 6)
		row_print(1, 1, "%s: <CALCID %d>", str_calc, calc);
	else
		row_print(1, 1, "%s: %s", str_calc, calc_names[calc]);

	if(mpu < 0 || mpu > 4)
		row_print(_(2,3), 1, "MPU: <MPUID %d>", mpu);
	else
		row_print(_(2,3), 1, "MPU: %s", mpu_names[mpu]);

	volatile uint32_t *CPUOPM = (void *)0xff2f0000;
	uint32_t SR;
	__asm__("stc sr, %0" : "=r"(SR));

	#ifdef FX9860G
	if(isSH3())
	{
		row_print(4, 1, " SR %08x", SR);
		return;
	}
	row_print(4, 1, "     SR %08x", SR);
	row_print(5, 1, "    PVR %08x", gint[HWCPUVR]);
	row_print(6, 1, "    PRR %08x", gint[HWCPUPR]);
	row_print(7, 1, " CPUOPM %08x", *CPUOPM);
	#endif

	#ifdef FXCG50
	row_print(4, 1, " Status Register: %08x", SR);
	row_print(5, 1, " Processor Version Register: %08x", gint[HWCPUVR]);
	row_print(6, 1, " Product Register: %08x", gint[HWCPUPR]);
	row_print(7, 1, " CPU Operation Mode: %08x", *CPUOPM);
	#endif
}

/* Memory */
static void show_memory(void)
{
	uint32_t base_rom  = 0x80000000;
	uint32_t base_ram  = 0x88000000;
	uint32_t base_uram = (uint32_t)mmu_uram();

	int rom  = gint[HWROM];
	int ram  = gint[HWRAM];
	int uram = mmu_uram_size();

	#ifdef FX9860G
	row_title("Basic memory layout");
	row_print(3, 2, "%08x %4dk ROM",  base_rom,   rom >> 10);
	row_print(4, 2, "%08x %4dk RAM",  base_ram,   ram >> 10);
	row_print(5, 2, "%08x %4dk URAM", base_uram, uram >> 10);
	row_print(6, 2, "Mapped   %4dk", gint[HWURAM] >> 10);
	#endif

	#ifdef FXCG50
	if(gint[HWCALC] == HWCALC_FXCG50) base_ram = 0x8c000000;
	row_print(1, 1, "ROM: %dM", rom >> 20);
	row_print(2, 2, "%08X ... %08X", 0x80000000, 0x80000000+rom-1);
	row_print(3, 1, "RAM: %dM", ram >> 20);
	row_print(4, 2, "%08X ... %08X", base_ram, base_ram+ram-1);
	row_print(5, 1, "Userspace RAM: %dk blocked", uram >> 10);
	row_print(6, 2, "%08X ... %08X", base_uram, base_uram+uram-1);
	row_print(7, 1, "Total mapped RAM: %dk", gint[HWURAM] >> 10);
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
