#include <gint/gint.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/drivers.h>
#include <gint/clock.h>
#include <gint/hardware.h>
#include <gint/mpu/tmu.h>
#include <gint/mpu/dma.h>
#include <gint/std/string.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

static int switches = 0;
static int fast = 0;
static int menus = 0;

static void switch_function(void)
{
	switches++;
}

//---
//	Saved system context visualizer
//---

void *driver_ctx(char const *name)
{
	extern gint_driver_t bdrv, edrv;
	for(gint_driver_t *drv = &bdrv; drv < &edrv; drv++)
	{
		if(!strcmp(drv->name, name)) return drv->sys_ctx;
	}
	return NULL;
}

#ifdef FX9860G
static void ctx_tmu()
{
	tmu_t *t = driver_ctx("TMU");
	uint8_t *TSTR = (void *)t + 3 * sizeof(tmu_t) + 6 * sizeof(etmu_t);

	for(int i = 0; i < 3; i++, t++)
	{
		row_print(2*i+1, 1, "TMU%d=%d   CNT:%08x", i,
			(*TSTR & (1 << i)) != 0, t->TCNT);
		row_print(2*i+2, 1, "TCR:%04x COR:%08x", t->TCR.word, t->TCOR);
	}
}
static void ctx_dd()
{
	uint8_t *STRD = driver_ctx("T6K11");

	row_print(1, 1, "STRD:%02x", *STRD);
	row_print(2, 2, "RESET:%d", (*STRD & 0x08) != 0);
	row_print(3, 2, "N/F:%d",   (*STRD & 0x04) != 0);
	row_print(4, 2, "X/Y:%d",   (*STRD & 0x02) != 0);
	row_print(5, 2, "U/D:%d",   (*STRD & 0x01) != 0);
}
static void ctx_rtc()
{
	uint8_t *ctx = driver_ctx("RTC");
	if(!ctx) return;

	uint8_t RCR1=ctx[0], RCR2=ctx[1];
	row_print(1, 1, "RCR1:%02x", RCR1);
	row_print(2, 1, "RCR2:%02x", RCR2);
}
static void show_etmu(int line, int id, etmu_t *t)
{
	row_print(line,   1, "ETMU%d=%-3dCNT:%08x", id, t->TSTR, t->TCNT);
	row_print(line+1, 1, "TCR:%02x   COR:%08x", t->TCR.byte, t->TCOR);
}
static void ctx_etmu(int start)
{
	etmu_t *t = driver_ctx("TMU") + 3 * sizeof(tmu_t);
	int length = 3;

	if(isSH3()) length=1, start=0;

	t += start;
	for(int i = 0; i < length; i++, t++)
	{
		show_etmu(2*i+1, i+start, t);
	}
}
#endif /* FX9860G */

#ifdef FXCG50
static void ctx_tmu()
{
	tmu_t *t = driver_ctx("TMU");
	etmu_t *e = (void *)(t + 3);
	uint8_t *TSTR = (void *)(e + 6);

	int const x[] = { 6, 138, 270, 6, 138, 270, 6, 138, 270 };
	int const y[] = { 24, 24, 24, 84, 84, 84, 144, 144, 144 };

	tmu_print(x[0], y[0], "TMU0", t+0, *TSTR & 0x1);
	tmu_print(x[1], y[1], "TMU1", t+1, *TSTR & 0x2);
	tmu_print(x[2], y[2], "TMU2", t+2, *TSTR & 0x4);

	etmu_print(x[3], y[3], "ETMU0", e+0);
	etmu_print(x[4], y[4], "ETMU1", e+1);
	etmu_print(x[5], y[5], "ETMU2", e+2);

	etmu_print(x[6], y[6], "ETMU3", e+3);
	etmu_print(x[7], y[7], "ETMU4", e+4);
	etmu_print(x[8], y[8], "ETMU5", e+5);
}
static void ctx_dd()
{
	uint16_t *win = driver_ctx("R61524");
	uint16_t HSA = win[0], HEA = win[1], VSA = win[2], VEA = win[3];

	row_print(1, 1, "Horizontal range: %d..%d", HSA, HEA);
	row_print(2, 1, "Vertical range: %d..%d", VSA, VEA);
}
static void ctx_rtc()
{
	uint8_t *ctx = driver_ctx("RTC");
	if(!ctx) return;

	uint8_t RCR1=ctx[0], RCR2=ctx[1];
	row_print(1, 1, "RCR1:%02x", RCR1);
	row_print(2, 1, "RCR2:%02x", RCR2);
}
static void ctx_dma()
{
	sh7305_dma_channel_t *ch = driver_ctx("DMA0");
	int *clock = (void *)(ch + 6);
	uint16_t *OR = (void *)(clock + 1);

	show_dma(6,   24, 0, ch+0);
	show_dma(138, 24, 1, ch+1);
	show_dma(270, 24, 2, ch+2);

	show_dma(6,   104, 3, ch+3);
	show_dma(138, 104, 4, ch+4);
	show_dma(270, 104, 5, ch+5);

	dprint(6, 184, C_BLACK, C_WHITE, "DMAOR: %08X", *OR);
	dprint(198, 184, C_BLACK, C_WHITE, "Clock enabled: %s",
		(*clock ? "No" : "Yes"));
}
#endif /* FXCG50 */

static void system_contexts(void)
{
	int key=0, tab=0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		extern image_t img_opt_switch_ctx_sh3;
		extern image_t img_opt_switch_ctx;

		if(isSH3())
			dimage(0, 56, &img_opt_switch_ctx_sh3);
		else
			dimage(0, 56, &img_opt_switch_ctx);
		#endif

		#ifdef FXCG50
		row_title("System context visualizer");
		fkey_button(1, "TMU");
		fkey_button(2, "R61524");
		fkey_button(3, "RTC");
		fkey_button(4, "DMA");
		#endif

		if(tab == 0) ctx_tmu();
		if(tab == 1) ctx_dd();
		if(tab == 2) ctx_rtc();

		#ifdef FX9860G
		if(tab == 3) ctx_etmu(0);
		if(tab == 4) ctx_etmu(3);
		#endif

		#ifdef FXCG50
		if(tab == 3) ctx_dma();
		#endif

		dupdate();

		key = getkey().key;
		if(key == KEY_F1) tab = 0;
		if(key == KEY_F2) tab = 1;
		if(key == KEY_F3) tab = 2;
		if(key == KEY_F4) tab = 3;

		#ifdef FX9860G
		if(key == KEY_F5 && isSH3()) tab = 4;
		#endif
	}
}

//---
//	Interactive in-out switching
//---

/* Render VRAM for this screen, used when returning to menu */
void render(void)
{
	dclear(C_WHITE);

	#ifdef FX9860G
	extern image_t img_opt_switch;
	row_print(1, 1, "Switch to OS");
	row_print(3, 1, "Switches done: %d", switches);
	row_print(4, 1, "Fast done: %d", fast);
	row_print(5, 1, "Menus done: %d", menus);

	dimage(0, 56, &img_opt_switch);
	#endif

	#ifdef FXCG50
	row_title("Hot switching between gint and OS");
	row_print(2, 1, "Switches done: %d", switches);
	row_print(3, 1, "Fast done: %d", fast);
	row_print(4, 1, "Menus done: %d", menus);
	fkey_button(1, "SWITCH");
	fkey_button(2, "FAST");
	fkey_button(3, "MENU");
	fkey_action(6, "SYSTEM");
	#endif
}

/* gintctl_gint_switch(): Test the gint switch-in-out procedures */
void gintctl_gint_switch(void)
{
	int key = 0;

	while(key != KEY_EXIT)
	{
		render();
		dupdate();

		key = getkey().key;
		if(key == KEY_F1) gint_switch(switch_function);
		/* TODO: Fast gint switch in gintctl */
		if(key == KEY_F2) {}
		/* Wait for F3 to be released before calling next getkey() */
		if(key == KEY_F3)
		{
			/* Render next frame in advance. When we return from
			   the main menu, our VRAM will be displayed but
			   GetKeyWait() will not give control back until a key
			   is pressed. This will make it look like we render a
			   new frame right away. */
			menus++;
			render();
			dupdate();

			gint_osmenu();
			while(keydown(KEY_F3)) waitevent(NULL);
		}
		if(key == KEY_F6) system_contexts();
	}
}
