#include <gint/mpu/tmu.h>
#include <gint/std/stdio.h>
#include <gint/timer.h>
#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/util.h>
#include <gintctl/gint.h>

/* tmu_print(): Print a TMU's details */
void tmu_print(int x, int y, char const *name, tmu_t *tmu, int running)
{
	int dx = _(27,45), dy = _(8,14);
	print(x, y, "%s:", name);
	if(!tmu)
	{
		print(x, y + dy, "(null)");
		return;
	}

	print(x, y + dy, "TCOR");
	print(x + dx, y + dy, "%08x", tmu->TCOR);

	print(x, y + 2 * dy, "TCNT");
	print(x + dx, y + 2 * dy, "%08x", tmu->TCNT);

	print(x, y + 3 * dy, "%s%s%s",
		tmu->TCR.UNIE ? "UNIE " : "",
		tmu->TCR.UNF  ? "UNF "  : "",
		running       ? "TSTR " : ""
	);
}

/* etmu_print(): Print an ETMU's details */
void etmu_print(int x, int y, char const *name, etmu_t *etmu)
{
	int dx = _(27,45), dy = _(8,14);
	print(x, y, "%s:", name);
	if(!etmu)
	{
		print(x, y + dy, "(null)");
		return;
	}

	print(x, y + dy, "TCOR");
	print(x + dx, y + dy, "%08x", etmu->TCOR);

	print(x, y + 2 * dy, "TCNT");
	print(x + dx, y + 2 * dy, "%08x", etmu->TCNT);

	print(x, y + 3 * dy, "%s%s%s",
		etmu->TCR.UNIE ? "UNIE " : "",
		etmu->TCR.UNF  ? "UNF "  : "",
		etmu->TSTR     ? "TSTR " : ""
	);
}

/* gintctl_gint_timer(): Show the timer status in real-time */
void gintctl_gint_timer(void)
{
	/* In this program we'll display the timer status at "full speed" and
	   hence ask getkey() to never wait. (The processor is still sleeping
	   during the DMA transfer to the screen on fxcg50, limiting the
	   program to ~90 FPS.) */
	int key = 0, timeout = 1;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		#warning gintctl_gint_timer not implemented yet
		#endif

		#ifdef FXCG50
		row_title("Timer status");

		volatile uint8_t *TSTR;
		timer_address(0, &TSTR);

		tmu_print(6,   24, "TMU0", timer_address(0, NULL),*TSTR & 0x1);
		tmu_print(138, 24, "TMU1", timer_address(1, NULL),*TSTR & 0x2);
		tmu_print(270, 24, "TMU2", timer_address(2, NULL),*TSTR & 0x4);

		etmu_print(6,    84, "ETMU0", timer_address(3, NULL));
		etmu_print(138,  84, "ETMU1", timer_address(4, NULL));
		etmu_print(270,  84, "ETMU2", timer_address(5, NULL));
		etmu_print(6,   144, "ETMU3", timer_address(6, NULL));
		etmu_print(138, 144, "ETMU4", timer_address(7, NULL));
		etmu_print(270, 144, "ETMU5", timer_address(8, NULL));

		fkey_button(1, "SLEEP");
		#endif

		dupdate();
		key = getkey_opt(GETKEY_DEFAULT, &timeout).key;


		/* On F1, pretend to sleep and just see what happens */
		if(key == KEY_F1)
		{
			volatile int flag = 0;

			int free = timer_setup(5, timer_delay(5, 1000000), 0,
				timer_timeout, &flag);
			if(free >= 0) timer_start(5);
		}
	}
}
