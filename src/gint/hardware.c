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
static void hw_mpucpu(int *row)
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
	int mpu = gint[HWMPU];

	put(_("MPU and CPU","MPU product and CPU features:"));
	if(mpu < 0 || mpu > 4) put(" <MPUID %d>", mpu);
	else put(" %s", mpu_names[mpu]);

	if(!isSH4()) return;
	put(_(" PVR:"," Processor Version Register: ") "%08x", gint[HWCPUVR]);
	put(_(" PRR:"," Product Register: ") "%08x", gint[HWCPUPR]);
}

/* Memory */
static void hw_memory(int *row)
{
	int mmu  = gint[HWMMU];
	int rom  = gint[HWROM];
	int ram  = gint[HWRAM];
	int uram = gint[HWURAM];

	put("Memory and MMU" _(,":"));
	load_barrier(mmu);

	put(" ROM:" _(," ") "%dM", rom >> 20);

	#ifdef FX9860G
	put(" RAM:%dk (%dk user)", ram >> 10, uram >> 10);
	#else
	put(" RAM: %dM (%dk mapped in userspace)", ram >> 20, uram >> 10);
	#endif

	if(mmu & HWMMU_UTLB) put(" TLB is unified");
	if(mmu & HWMMU_FITTLB) put(
		_(" Add-in fits in TLB"," Add-in is fully mapped in the TLB"));
}

/* Clock Pulse generator */
static void hw_cpg(int *row)
{
	int cpg = gint[HWCPG];

	put(_("Clock Generator", "Clock Pulse Generator:"));
	load_barrier(cpg);

	if(cpg & HWCPG_COMP) put(
		_(" Input freq known"," Input clock frequency is known"));
	if(cpg & HWCPG_EXT) put(
		_(" SH7724-style CPG"," SH7724-style extended module"));
}

/* Direct Memory Access Controller */
static void hw_dma(int *row)
{
	int dma = gint[HWDMA];

	put("Direct Memory Access" _(," Controller:"));
	load_barrier(dma);

	put(" (loaded)");
}

/* Timer Unit */
static void hw_tmu(int *row)
{
	int tmu = gint[HWTMU];

	put("Timer Unit" _(,":"));
	load_barrier(tmu);

	put(" (loaded)");
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

/* Real-Time Clock */
static void hw_rtc(int *row)
{
	int rtc = gint[HWRTC];

	put("Real-Time Clock" _(,":"), rtc);
	load_barrier(rtc);

	if(rtc & HWRTC_TIMER) put(" timer enabled");
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

	hw_mpucpu(row);
	put("");

	hw_memory(row);
	put("");

	hw_cpg(row);
	put("");

	hw_dma(row);
	put("");

	hw_tmu(row);
	hw_etmu(row);
	put("");

	hw_rtc(row);
	put("");

	hw_keyboard(row);
	put("");

	hw_display(row);

	return row_value + offset;
}

/* gintctl_hardware(): Show the hardware screen */
void gintctl_gint_hardware(void)
{
	int offset = 0;
	int max, key = 0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FXCG50
		row_title("Hardware and loaded drivers");
		#endif

		max = display_data(offset);
		scrollbar(offset, max, 1, row_count() + 1);

		dupdate();

		key = getkey().key;

		if(key == KEY_UP && offset > 0) offset--;
		if(key == KEY_DOWN && max - offset > row_count()) offset++;
	}
}
