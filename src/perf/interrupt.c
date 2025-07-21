#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/timer.h>

#include <gintctl/perf.h>
#include <gintctl/util.h>

#include <gint/prof.h>

#define STRESS_LIMIT 1000

int stress_callback(int *counter)
{
	(*counter)++;
	if(*counter >= STRESS_LIMIT) return TIMER_STOP;
	return TIMER_CONTINUE;
}

uint32_t stress_interrupts(void)
{
	int counter = 0;
	int timer = timer_configure(TIMER_ANY, 1,
		GINT_CALL(stress_callback, &counter));
	if(timer < 0) return 0;

	return prof_exec({
		timer_start(timer);
		timer_wait(timer);
	});
}

/* gintctl_perf_interrupts(): Interrupt handling */
void gintctl_perf_interrupts(void)
{
	uint32_t time_spent = stress_interrupts();
	int rate = 0;
	int key = 0;

	if(time_spent > 0)
	{
		rate = 1000000ull * STRESS_LIMIT / time_spent;
	}

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#if GINT_RENDER_MONO
		row_title("Interrupt stress");

		if(time_spent == 0)
		{
			row_print(3, 1, "Not tested");
		}
		else
		{
			row_print(3, 1, "Count: %d", STRESS_LIMIT);
			row_print(4, 1, "Time:  %d us", time_spent);
			row_print(5, 1, "Rate:  %d int/s", rate);
		}
		#endif

		#if GINT_RENDER_RGB
		row_title("Interrupt handling stress test");

		if(time_spent == 0)
		{
			row_print(1, 1, "Not tested");
		}
		else
		{
			row_print(1, 1, "%d interrupts handled in %d us",
				STRESS_LIMIT, time_spent);
			row_print(2, 1, "Rate is %d int/s", rate);
		}
		#endif

		dupdate();
		key = getkey().key;
	}
}
