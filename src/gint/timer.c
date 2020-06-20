#include <gint/mpu/tmu.h>
#include <gint/std/stdio.h>
#include <gint/timer.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/clock.h>

#include <gintctl/util.h>
#include <gintctl/gint.h>

/* timer_print(): Print a timer's details */
void timer_print(int x, int y, char const *name, uint32_t TCOR, uint32_t TCNT,
	int UNIE, int UNF, int STR)
{
	int dy = _(8,14);
	dprint(x, y, C_BLACK, "%s:", name);

	#ifdef FXCG50
	dprint(x, y+dy,   C_BLACK, "TCOR");
	dprint(x, y+2*dy, C_BLACK, "TCNT");
	#endif

	dprint(_(x+6, x+45), y+dy, C_BLACK, "%08X", TCOR);
	dprint(_(x+60, x+45), _(y+dy, y+2*dy), C_BLACK, "%08X", TCNT);

	dprint(_(x+36, x), _(y, y+3*dy), C_BLACK, "%s%s%s",
		UNIE ? "UNIE " : "",
		UNF  ? "UNF "  : "",
		STR  ? "STR "  : ""
	);
}

/* tmu_print(): Print a TMU's details */
void tmu_print(int x, int y, char const *name, tmu_t *tmu, int STR)
{
	timer_print(x, y, name, tmu->TCOR, tmu->TCNT, tmu->TCR.UNIE,
		tmu->TCR.UNF, STR);
}

/* etmu_print(): Print an ETMU's details */
void etmu_print(int x, int y, char const *name, etmu_t *etmu)
{
	timer_print(x, y, name, etmu->TCOR, etmu->TCNT, etmu->TCR.UNIE,
		etmu->TCR.UNF, etmu->TSTR);
}


#ifdef FX9860G
static int x[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
static int y[] = { 0, 16, 32, 0, 16, 32, 0, 16, 32 };
#else
static int x[] = { 6, 138, 270, 6, 138, 270, 6, 138, 270 };
static int y[] = { 24, 24, 24, 84, 84, 84, 144, 144, 144 };
#endif

void show_tmu(void)
{
	tmu_t *TMU = SH7305_TMU.TMU;
	volatile uint8_t *TSTR = &SH7305_TMU.TSTR;

	if(isSH3())
	{
		TMU = SH7705_TMU.TMU;
		TSTR = &SH7705_TMU.TSTR;
	}

	tmu_print(x[0], y[0], "TMU0", &TMU[0], *TSTR & 0x1);
	tmu_print(x[1], y[1], "TMU1", &TMU[1], *TSTR & 0x2);
	tmu_print(x[2], y[2], "TMU2", &TMU[2], *TSTR & 0x4);
}

void show_etmu_1(void)
{
	etmu_t *ETMU = isSH3() ? SH7705_ETMU : SH7305_ETMU;
	etmu_print(x[3], y[3], "ETMU0", &ETMU[0]);

	if(isSH3()) return;
	etmu_print(x[4], y[4], "ETMU1", &ETMU[1]);
	etmu_print(x[5], y[5], "ETMU2", &ETMU[2]);
}

void show_etmu_2(void)
{
	etmu_t *ETMU = SH7305_ETMU;

	if(isSH3()) return;
	etmu_print(x[6], y[6], "ETMU3", &ETMU[3]);
	etmu_print(x[7], y[7], "ETMU4", &ETMU[4]);
	etmu_print(x[8], y[8], "ETMU5", &ETMU[5]);
}

/* gintctl_gint_timer(): Show the timer status in real-time */
void gintctl_gint_timer(void)
{
	/* In this program we'll display the timer status at "full speed" and
	   hence ask getkey() to never wait. (The processor is still sleeping
	   during the DMA transfer to the screen on fxcg50, limiting the
	   program to ~90 FPS.) */
	int key=0, tid=0;
	GUNUSED int timeout=1;
	volatile int flag = 0;

	#ifdef FX9860G
	int tab = 1;
	#endif

	while(!keydown(KEY_EXIT))
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		if(tab == 1) show_tmu();
		if(tab == 2) show_etmu_1();
		if(tab == 3) show_etmu_2();

		extern bopti_image_t img_opt_gint_timers;
		dimage(0, 56, &img_opt_gint_timers);

		if(tid < 3) dprint(23, 56, C_BLACK, "TMU%d", tid);
		else        dprint(23, 56, C_BLACK, "ETMU%d", tid-3);
		#endif

		#ifdef FXCG50
		row_title("Timer status");

		show_tmu();
		show_etmu_1();
		show_etmu_2();

		fkey_action(1, "SLEEP");

		if(tid < 3) dprint(72, 210, C_BLACK, "TMU%d", tid);
		else        dprint(72, 210, C_BLACK, "ETMU%d", tid-3);
		#endif

		dupdate();

		int timeout = 1;
		key_event_t ev = getkey_opt(GETKEY_DEFAULT, &timeout);
		if(ev.type == KEYEV_NONE) continue;
		key = ev.key;

		/* On F1, pretend to sleep and just see what happens */
		if(key == KEY_F1)
		{
			int free = timer_setup(tid, timer_delay(tid, 1000000,
				TIMER_Pphi_4), NULL);
			if(free == tid) timer_start(tid);
		}

		#ifdef FX9860G
		/* On F4, F5 and F6, switch tabs */
		if(key == KEY_F4) tab = 1;
		if(key == KEY_F5) tab = 2;
		if(key == KEY_F6) tab = 3;
		#endif

		if(key == KEY_UP) tid = (tid+timer_count()-1) % timer_count();
		if(key == KEY_DOWN) tid = (tid + 1) % timer_count();
	}
}
