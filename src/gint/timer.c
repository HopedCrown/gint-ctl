#include <gint/mpu/tmu.h>
#include <gint/std/stdio.h>
#include <gint/timer.h>
#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/util.h>
#include <gintctl/gint.h>

/* timer_print(): Print a timer's details */
void timer_print(int x, int y, char const *name, uint32_t TCOR, uint32_t TCNT,
	int UNIE, int UNF, int STR)
{
	int dy = _(8,14);
	print(x, y, "%s:", name);

	#ifdef FXCG50
	print(x, y+dy,   "TCOR");
	print(x, y+2*dy, "TCNT");
	#endif

	print(_(x+6, x+45), y+dy, "%08X", TCOR);
	print(_(x+60, x+45), _(y+dy, y+2*dy), "%08X", TCNT);

	print(_(x+36, x), _(y, y+3*dy), "%s%s%s",
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
	volatile uint8_t *TSTR;
	timer_address(0, &TSTR);

	tmu_print(x[0], y[0], "TMU0", timer_address(0, NULL), *TSTR & 0x1);
	tmu_print(x[1], y[1], "TMU1", timer_address(1, NULL), *TSTR & 0x2);
	tmu_print(x[2], y[2], "TMU2", timer_address(2, NULL), *TSTR & 0x4);
}

void show_etmu_1(void)
{
	etmu_print(x[3], y[3], "ETMU0", timer_address(3, NULL));

	if(isSH3()) return;
	etmu_print(x[4], y[4], "ETMU1", timer_address(4, NULL));
	etmu_print(x[5], y[5], "ETMU2", timer_address(5, NULL));
}

void show_etmu_2(void)
{
	if(isSH3()) return;

	etmu_print(x[6], y[6], "ETMU3", timer_address(6, NULL));
	etmu_print(x[7], y[7], "ETMU4", timer_address(7, NULL));
	etmu_print(x[8], y[8], "ETMU5", timer_address(8, NULL));
}

/* gintctl_gint_timer(): Show the timer status in real-time */
void gintctl_gint_timer(void)
{
	/* In this program we'll display the timer status at "full speed" and
	   hence ask getkey() to never wait. (The processor is still sleeping
	   during the DMA transfer to the screen on fxcg50, limiting the
	   program to ~90 FPS.) */
	int key = 0, timeout = 1;

	#ifdef FX9860G
	int tab = 1;
	#endif

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		if(tab == 1) show_tmu();
		if(tab == 2) show_etmu_1();
		if(tab == 3) show_etmu_2();

		extern image_t img_opt_gint_timers;
		dimage(0, 56, &img_opt_gint_timers);
		#endif

		#ifdef FXCG50
		row_title("Timer status");

		show_tmu();
		show_etmu_1();
		show_etmu_2();

		fkey_button(1, "SLEEP");
		#endif

		dupdate();
		key = getkey_opt(GETKEY_DEFAULT, &timeout).key;

		/* On F1, pretend to sleep and just see what happens */
		if(key == KEY_F1)
		{
			volatile int flag = 0;
			int tid = 0;

			int free = timer_setup(tid, timer_delay(tid, 1000000),
				0, timer_timeout, &flag);
			if(free == tid) timer_start(tid);
		}

		#ifdef FX9860G
		/* On F4, F5 and F6, switch tabs */
		if(key == KEY_F4) tab = 1;
		if(key == KEY_F5) tab = 2;
		if(key == KEY_F6) tab = 3;
		#endif
	}
}
