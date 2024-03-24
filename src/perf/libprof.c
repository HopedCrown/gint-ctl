#include <gint/keyboard.h>
#include <gint/display.h>
#include <gint/timer.h>

#include <gintctl/util.h>
#include <gintctl/perf.h>

#include <libprof.h>

/* Waits some time and returns the libprof output in microseconds */
static uint32_t run_sleep(int us)
{
	/* We can't use sleep_us() as we want a TMU */
	volatile int flag = 0;
	int timer = timer_configure(TIMER_TMU, us, GINT_CALL_SET_STOP(&flag));
	if(timer < 0) return 0;

	return prof_exec({
		timer_start(timer);
		timer_wait(timer);
	});
}

/* Measure overhead of an empty context */
static uint32_t run_empty(void)
{
	return prof_exec();
}

/* gintctl_perf_libprof(): Test the libprof implementation */
void gintctl_perf_libprof(void)
{
	int key=0, test=0, delay=10000;

	uint32_t sleep_delay = 0;
	uint32_t empty = 0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#if GINT_RENDER_MONO
		row_print(1, 1, "Measures time for");
		row_print(2, 1, "10ms sleep +1us each");
		row_print(3, 1, "time, and empty code.");

		if(test)
		{
			row_print(5, 1, "Sleep: %.3D ms", sleep_delay);
			row_print(6, 1, "Empty: %d us", empty);
		}

		extern bopti_image_t img_opt_perf_libprof;
		dimage(0, 56, &img_opt_perf_libprof);
		#endif

		#if GINT_RENDER_RGB
		row_title("libprof basics");
		row_print(1, 1, "This program shows the execution time "
			"measured");
		row_print(2, 1, "by libprof for a 100 ms sleep, with 1 us "
			"added");
		row_print(3, 1, "each time.");
		row_print(5, 1, "Press F1 to start the test.");

		if(test)
		{
			row_print(7, 1, "Sleep: %.3D ms", sleep_delay);
			row_print(8, 1, "Empty: %d us", empty);
			row_print(9, 1, "Tests: %d", test);
		}

		fkey_button(1, "START");
		#endif

		dupdate();
		key = getkey().key;

		if(key == KEY_F1)
		{
			sleep_delay = run_sleep(delay);
			empty = run_empty();

			delay++;
			test++;
		}
	}
}
