#include <gint/keyboard.h>
#include <gint/display.h>
#include <gint/clock.h>

#include <gintctl/util.h>
#include <gintctl/prof-contexts.h>
#include <gintctl/perf.h>

#include <libprof.h>

/* Waits 1s and returns the libprof output in microseconds */
static uint32_t run_test(void)
{
	int ctx = PROFCTX_BASICS;

	prof_clear(ctx);

	prof_enter(ctx);

	static int delay = 100000;
	sleep_us(1, delay);
	prof_leave(ctx);

	delay += 256;
	return prof_time(ctx);
}

/* gintctl_perf_libprof(): Test the libprof implementation */
void gintctl_perf_libprof(void)
{
	int key = 0, test = 0;
	uint32_t elapsed = 0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);
		row_title("libprof basics");

		#ifdef FX9860G
		#warning gintctl_perf_libprof incomplete on fx9860g

		row_print(2, 2, "Checks that libprof");
		row_print(3, 2, "Press F1 to start.");
		row_print(5, 2, "Delay: 1s");

		if(test == 1) row_print(6, 2, "Elapsed: %8xus", elapsed);
		#endif

		#ifdef FXCG50
		row_print(1, 1, "This program shows the execution time "
			"measured");
		row_print(2, 1, "by libprof for a 100 ms sleep, with 256us "
			"added");
		row_print(3, 1, "each time.");
		row_print(5, 1, "Press F1 to start the test.");

		if(test) row_print(7, 1, "Elapsed: %.1j ms (%#08x us)",
			elapsed / 100, elapsed);
		if(test) row_print(8, 1, "Tests: %d", test);

		fkey_button(1, "START");
		#endif

		dupdate();
		key = getkey().key;

		if(key == KEY_F1) elapsed = run_test(), test++;
	}
}
