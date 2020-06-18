#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <gint/clock.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

static int callback_simple(volatile void *arg)
{
	/* Perform a multiplication to check basic register saves */
	int base = *(volatile int *)arg;
	return base * 387 + 1;
}

static int callback_sleep(GUNUSED volatile void *arg)
{
	sleep();
	return 1;
}

static int callback_timer(GUNUSED volatile void *arg)
{
	volatile int timeout = 0;

	timer_setup(1, timer_delay(1, 10000), timer_default, timer_timeout,
		&timeout);
	timer_start(1);
	timer_wait(1);
	return 1;
}

/* gintctl_gint_timer_callbacks(): Stunts in the environment of callbacks */
void gintctl_gint_timer_callbacks(void)
{
	int key = 0;
	int base=0, done=0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		row_title("Timer callbacks");

		row_print(3, 1, "F1:Simple callback");
		row_print(4, 1, "F2:Wait with sleep()");
		row_print(5, 1, "F3:Start timer!");

		extern bopti_image_t img_opt_gint_timer_callbacks;
		dimage(0, 56, &img_opt_gint_timer_callbacks);
		dprint(69, 56, C_BLACK, "Done:%d", done);
		#endif

		#ifdef FXCG50
		row_title("Interrupt management in timer callbacks");

		row_print(1, 1,
			"SIMPLE runs a short callback that modifies CPU");
		row_print(2, 1,
			"registers.");
		row_print(4, 1,
			"SLEEP transitions to standby mode, forcing an");
		row_print(5, 1,
			"interrupt within the callback.");
		row_print(7, 1,
			"TIMER runs a callback that starts a timer (with its");
		row_print(8, 1,
			"own callback) and waits for the interrupt.");

		row_print(10, 1, "Tests run: %d", done);

		fkey_action(1, "SIMPLE");
		fkey_action(2, "SLEEP");
		fkey_action(3, "TIMER");
		#endif

		dupdate();
		key = getkey().key;

		int (*callback)(volatile void *arg) = NULL;
		volatile void *arg = NULL;

		if(key == KEY_F1) callback = callback_simple, arg = &base;
		if(key == KEY_F2) callback = callback_sleep;
		if(key == KEY_F3) callback = callback_timer;

		if(callback)
		{
			timer_setup(0, timer_delay(0, 10000), timer_default,
				callback, arg);
			timer_start(0);
			timer_wait(0);
			done++;
		}
	}
}

