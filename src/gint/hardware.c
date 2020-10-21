#include <gint/hardware.h>
#include <gint/keyboard.h>
#include <gint/display.h>

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
		"SH-3 fx-9860G-like",
		"SH-4 fx-9860G-like",
		"Graph 35+E II",
		"Prizm fx-CG 20",
		"fx-CG 50/Graph 90+E",
		"fx-CG Manager",
	};

	int mpu  = gint[HWMPU];
	int calc = gint[HWCALC];

	if(calc < 0 || calc > 6)
		row_print(1, 1, "Calculator model: <CALCID %d>", calc);
	else
		row_print(1, 1, "Calculator model: %s", calc_names[calc]);

	if(mpu < 0 || mpu > 4)
		row_print(3, 1, "MPU: <MPUID %d>", mpu);
	else
		row_print(3, 1, "MPU: %s", mpu_names[mpu]);

	if(!isSH4()) return;

	row_print(4, 1, _(" PVR:"," Processor Version Register: ") "%08x",
		gint[HWCPUVR]);
	row_print(5, 1, _(" PRR:"," Product Register: ") "%08x",gint[HWCPUPR]);

	volatile uint32_t *CPUOPM = (void *)0xff2f0000;
	row_print(6, 1, _(" CPUOPM:"," CPU Operation Mode: ") "%08x", *CPUOPM);

	uint32_t SR;
	__asm__("stc sr, %0" : "=r"(SR));
	row_print(7, 1, _(" SR:", " Status Register: ") "%08x", SR);
}

/* Memory */
static void show_memory(void)
{
	int rom  = gint[HWROM];
	int ram  = gint[HWRAM];
	int uram = gint[HWURAM];

	#ifdef FX9860G
	row_print(1, 1, "ROM: %dM", rom >> 20);
	row_print(2, 1, "RAM:%dk (%dk user)", ram >> 10, uram >> 10);
	#endif

	#ifdef FXCG50
	row_print(1, 1, "ROM: %dM", rom >> 20);
	row_print(2, 2, "%08X .. %08X", 0x80000000, 0x80000000+rom-1);
	row_print(3, 1, "RAM: %dM (%dk mapped in userspace)",
		ram >> 20, uram >> 10);
	#endif
}

/* Extra Timer Unit */
static void hw_etmu(int *row)
{
	int etmu = gint[HWETMU];

	put("Extra Timer Unit" _(,":"));
	load_barrier(etmu);

	if(etmu & HWETMU_1)
	{
		put(" Extra timers: 1");
		put(" Operational: %c", (etmu & HWETMU_OK0 ? 'y' : 'n'));
	}
	else if(etmu & HWETMU_6)
	{
		char operational[7] = { 0 };
		for(int i = 0; i < 6; i++)
			operational[i] = etmu & (1 << (i + 2)) ? 'y' : 'n';

		put(" Extra timers: 6");
		put(" Operational: %s", operational);
	}
}

/* Keyboard */
static void hw_keyboard(int *row)
{
	int kbd = gint[HWKBD];

	put("Keyboard" _(,":"), kbd);
	load_barrier(kbd);

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

	put(_(" Scans at %dHz"," Scans keys at %dHz"), gint[HWKBDSF]);
}

/* Display driver */
static void hw_display(int *row)
{
	int dd = gint[HWDD];

	put("Display" _(,":"));
	load_barrier(dd);

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

static int display_data(int offset)
{
	int row_value = -offset;
	int *row = &row_value;

	hw_etmu(row);
	put("");

	hw_keyboard(row);
	put("");

	hw_display(row);

	return row_value + offset;
}

/* gintctl_hardware(): Show the hardware screen */
void gintctl_gint_hardware(void)
{
	int offset = 0, tab = 0;
	int max, key = 0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		if(tab == 0) show_mpucpu();
		if(tab == 1) show_memory();
		if(tab == 2)
		{
			max = display_data(offset);
			scrollbar(offset, max, 1, row_count() + 1);
		}

		#ifdef FXCG50
		row_title("Hardware and loaded drivers");
		fkey_menu(1, "MPU/CPU");
		fkey_menu(2, "MEMORY");
		fkey_menu(3, "Other");
		#endif

		dupdate();

		key = getkey().key;

		if(key == KEY_F1) tab = 0;
		if(key == KEY_F2) tab = 1;
		if(key == KEY_F3) tab = 2;

		if(tab == 2 && key == KEY_UP && offset > 0)
			offset--;
		if(tab == 2 && key == KEY_DOWN && max - offset > row_count())
			offset++;
	}
}
