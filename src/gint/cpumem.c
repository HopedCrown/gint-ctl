#include <gint/hardware.h>
#include <gint/keyboard.h>
#include <gint/display.h>
#include <gint/mmu.h>
#include <gint/config.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>
#include <gintctl/assets.h>
#include <gintctl/ui.h>

#include <stdio.h>

/* TODO: Include <gint/cpu.h> */
extern uint32_t cpu_getVBR(void);

/* Some symbols from the linker script */
extern uint32_t
	brom, srom,			/* Limits of ROM mappings */
	sdata,  rdata,			/* User's data section */
	sbss, rbss,			/* User's BSS section */
	sgmapped;			/* Permanently mapped functions */


/* MPU type and processor version */
void show_mpucpu(jlabel *label)
{
	char const *mpu_names[] = {
		#if GINT_RENDER_MONO
		"Unknown",
		"SH-3 SH7337",
		"SH-4A SH7305",
		"SH-3 SH7355",
		"SH-4A SH7724",
		#elif GINT_RENDER_RGB
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

	/* Generate a default calc name if invalid values are found */
	char calc_default[16];
	sprintf(calc_default, "<CALCID %d>", calc);
	char const *str_calc = (uint)calc < 7 ? calc_names[calc] : calc_default;

	/* Generate a default MPU name if invalid values are found */
	char mpu_default[16];
	sprintf(mpu_default, "<MPUID %d>", mpu);
	char const *str_mpu = (uint)mpu < 5 ? mpu_names[mpu] : mpu_default;

	/* Generate a default fs name if invalid values are found */
	char fs_default[16];
	sprintf(fs_default, "<FSID %d>", fs);
	char const *str_fs = (uint)fs < 3 ? fs_names[fs] : fs_default;

	volatile uint32_t *CPUOPM = (void *)0xff2f0000;
	uint32_t SR, r15;
	__asm__("stc sr,  %0" : "=r"(SR));
	__asm__("mov r15, %0" : "=r"(r15));

	#if GINT_RENDER_MONO
	jlabel_asprintf(label,
		"Model: %s\n"
		"MPU: %s\n"
		"Filesystem: %s\n"
		"SR:  %08X | VBR: %08X\n"
		"PVR: %08X | PRR: %08X\n"
		"R15: %08X | CPUOPM: %08X",
		str_calc, str_mpu, str_fs, SR, cpu_getVBR(),
		isSH3() ? (uint)-1 : gint[HWCPUVR],
		isSH3() ? (uint)-1 : gint[HWCPUPR], r15,
		isSH3() ? (uint)-1 : *CPUOPM);
	#elif GINT_RENDER_RGB
	jlabel_asprintf(label,
		"Calculator model: %s\n"
		"MPU: %s\n"
		" Status Register: %08x\n"
		" Processor Version Register: %08x\n"
		" Product Register: %08x\n"
		" CPU Operation Mode: %08x\n"
		" Current VBR: %08x\n"
		" Current stack pointer: %08x\n"
		"\n"
		"Filesystem type: %s",
		str_calc, str_mpu, SR, gint[HWCPUVR], gint[HWCPUPR], *CPUOPM,
		cpu_getVBR(), r15, str_fs);
	#endif
}

/* Memory */
static void show_memory(jlabel *label)
{
	#if GINT_RENDER_MONO
	jlabel_asprintf(label,
		"ROM: %dk, RAM: %dk\n"
		"User RAM: %08X (%dk, P0 %dk)\n"
		">rom:   %08X +%06d\n"
		">data:  %08X +%06d\n"
		">bss:   %08X +%06d\n"
		">reloc: %08X +%06d",
		gint[HWROM] >> 10, gint[HWRAM] >> 10,
		mmu_uram(), mmu_uram_size() >> 10, gint[HWURAM] >> 10,
		&brom, &srom, &rdata, &sdata, &rbss, &sbss, mmu_uram(), &sgmapped);
	#endif

	#if GINT_RENDER_RGB
	uint32_t base_ram  = 0x88000000;
	if(gint[HWCALC] == HWCALC_FXCG50) base_ram = 0x8c000000;

	jlabel_asprintf(label,
		"RAM: %dM, RAM: %dM (starts at %08X)\n"
		"Userspace RAM: %08X (%dk continuous block)\n"
		"Total RAM mapped in P0: %dk\n"
		"\n"
		">ROM:  %08X (+%06d)\n"
		">DATA: %08X (+%06d)\n"
		">BSS:  %08X (+%06d)\n",
		gint[HWROM] >> 20, gint[HWRAM] >> 20, base_ram,
		mmu_uram(), mmu_uram_size() >> 10,
		gint[HWURAM] >> 10,
		&brom, &srom, &rdata, &sdata, &rbss, &sbss);
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
	gscreen *s = gscreen_create2("CPU and memory", &img_opt_gint_cpumem,
		"Processor and memory", "/MPU/CPU;/MEMORY;;;;", NULL);
	gintctl_scene_push(s);

	jlabel *label_cpu = jlabel_create("<cpu>", NULL);
	jlabel_set_font(label_cpu, _(&font_mini, dfont_default()));
	show_mpucpu(label_cpu);
	gscreen_add_tab(s, label_cpu, NULL);

	jlabel *label_mem = jlabel_create("<mem>", NULL);
	jlabel_set_font(label_mem, _(&font_mini, dfont_default()));
	show_memory(label_mem);
	gscreen_add_tab(s, label_mem, NULL);

	while(true) {
		jevent e = jscene_run(gintctl_scene());
		if(jevent_is_press(e, KEY_EXIT))
			break;
		if(e.type == JFKEYS_TRIGGERED && e.data == 0)
			gscreen_show_tab(s, 0);
		if(e.type == JFKEYS_TRIGGERED && e.data == 1)
			gscreen_show_tab(s, 1);
	}
}
