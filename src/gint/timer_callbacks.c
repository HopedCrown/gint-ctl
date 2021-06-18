#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <gint/clock.h>
#include <gint/defs/util.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

static int tests = 0;
/* Auxiliary timer for tests than need an interrupt during the callback */
static int auxiliary_timer = -1;

static int callback_simple(volatile int *base)
{
	/* Perform a multiplication to check basic register saves */
	tests++;
	return (*base) * 387 + TIMER_STOP;
}

static int callback_sleep(void)
{
	/* Start the aux. timer to have a guaranteed non-masked interrupt */
	timer_start(auxiliary_timer);
	sleep();
	tests++;
	return TIMER_STOP;
}

static int callback_timer(void)
{
	/* Wait specifically on the auxiliary timer */
	timer_start(auxiliary_timer);
	timer_wait(auxiliary_timer);
	tests++;
	return TIMER_STOP;
}

static int callback_ics(cpu_csleep_t *ics)
{
	cpu_csleep_cancel(ics);
	tests++;
	return TIMER_STOP;
}

/* gintctl_gint_timer_callbacks(): Stunts in the environment of callbacks */
void gintctl_gint_timer_callbacks(void)
{
	int key=0, base=0;
	int status=0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#ifdef FX9860G
		row_title("Timer callbacks");

		row_print(3, 1, "F1:Simple callback");
		row_print(4, 1, "F2:Wait with sleep()");
		row_print(5, 1, "F3:Start timer!");

		if(status == 1) row_print(7, 1, "Success!");
		if(status == 2) row_print(7, 1, "Not enough timers!");

		extern bopti_image_t img_opt_gint_timer_callbacks;
		dimage(0, 56, &img_opt_gint_timer_callbacks);
		dprint(86, 56, C_BLACK, "Done:%d", tests);
		#endif

		#ifdef FXCG50
		row_title("Interrupt management in timer callbacks");

		row_print(1, 1,
			"SIMPLE runs a short callback that modifies CPU");
		row_print(2, 1,
			"registers.");
		row_print(4, 1,
			"SLEEP transitions to sleep mode, forcing an");
		row_print(5, 1,
			"interrupt within the callback.");
		row_print(7, 1,
			"TIMER runs a callback that starts a timer (with its");
		row_print(8, 1,
			"own callback) and waits for the interrupt.");
		row_print(10, 1,
			"ICS uses an interrupt-cancellable sleep.");

		row_print(12, 1, "Tests run: %d", tests);

		if(status == 1) row_print(13, 1, "Success!");
		if(status == 2) row_print(13, 1, "Not enough timers!");

		fkey_button(1, "SIMPLE");
		fkey_button(2, "SLEEP");
		fkey_button(3, "TIMER");
		fkey_button(4, "ICS");
		#endif

		dupdate();
		key = getkey().key;

		status = 0;
		gint_call_t cb;
		auxiliary_timer = -1;

		cpu_csleep_t ics;

		if(key == KEY_F4)
		{
			cpu_csleep_init(&ics);
			cb = GINT_CALL(callback_ics, (void *)&ics);
		}
		else if(key == KEY_F1) cb = GINT_CALL(callback_simple, &base);
		else if(key == KEY_F2) cb = GINT_CALL(callback_sleep);
		else if(key == KEY_F3) cb = GINT_CALL(callback_timer);
		else continue;

		/* Allocate a first timer to run the callback */
		int t = timer_configure(TIMER_ANY, 40000, cb);
		if(t < 0)
		{
			status = 2;
			continue;
		}

		/* Now allocate a second timer for tests 2 and 3 */
		if(key == KEY_F2 || key == KEY_F3)
		{
			/* Request a TMU (higher priority) */
			auxiliary_timer = timer_configure(TIMER_TMU,10000,GINT_CALL_NULL);
			if(auxiliary_timer < 0)
			{
				status = 2;
				timer_stop(t);
				continue;
			}

			/* The auxiliary timer must have a higher priority */
			if(auxiliary_timer > t) swap(t, auxiliary_timer);
		}

		status = 1;
		timer_start(t);

		/* Test the ICS on F4 */
		if(key == KEY_F4) cpu_csleep(&ics);
		else timer_wait(t);
	}
}

