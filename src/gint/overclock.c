#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/clock.h>
#include <gint/timer.h>
#include <gint/mpu/cpg.h>
#include <gint/mpu/bsc.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

#include <libprof.h>
#include <stdio.h>

#if (GINT_HW_CG || GINT_HW_CP) && GINT_RENDER_RGB

#define CPG SH7305_CPG
#define BSC SH7305_BSC

char const *level_string(int level)
{
	static char str[32];

	if(level == CLOCK_SPEED_UNKNOWN)
		return "UNKNOWN";
	if(level == CLOCK_SPEED_F1)
		return "Ftune's F1";
	if(level == CLOCK_SPEED_F2)
		return "Ftune's F2";
	if(level == CLOCK_SPEED_F3)
		return "Ftune's F3";
	if(level == CLOCK_SPEED_F4)
		return "Ftune's F4";
	if(level == CLOCK_SPEED_F5)
		return "Ftune's F5";

	snprintf(str, 32, "<Level %d>", level);
	return str;
}

static int callback(volatile int *counter, volatile int *update)
{
	(*counter)++;
	*update = 1;
	return TIMER_CONTINUE;
}

void gintctl_gint_overclock(void)
{
	int key = 0;
	volatile int counter = 0;
	volatile int update = 0;

	int t = timer_configure(TIMER_TMU, 100000,
		GINT_CALL(callback, &counter, &update));
	if(t >= 0)
		timer_start(t);

	while(key != KEY_EXIT) {
		int level = clock_get_speed();

		uint32_t time_dclear = prof_exec({
			dclear(C_WHITE);
		});
		row_title("Clock speed detection and setting");

		row_print( 1, 29, "FLLFRQ:");
		row_print( 2, 29, "FRQCR:");
		row_print( 3, 29, "CS0BCR:");
		row_print( 4, 29, "CS2BCR:");
		row_print( 5, 29, "CS3BCR:");
		row_print( 6, 29, "CS5aBCR:");
		row_print( 7, 29, "CS0WCR:");
		row_print( 8, 29, "CS2WCR:");
		row_print( 9, 29, "CS3WCR:");
		row_print(10, 29, "CS5aWCR:");

		row_print( 1, 38, "0x%08x", CPG.FLLFRQ.lword);
		row_print( 2, 38, "0x%08x", CPG.FRQCR.lword);
		row_print( 3, 38, "0x%08x", BSC.CS0BCR.lword);
		row_print( 4, 38, "0x%08x", BSC.CS2BCR.lword);
		row_print( 5, 38, "0x%08x", BSC.CS3BCR.lword);
		row_print( 6, 38, "0x%08x", BSC.CS5ABCR.lword);
		row_print( 7, 38, "0x%08x", BSC.CS0WCR.lword);
		row_print( 8, 38, "0x%08x", BSC.CS2WCR.lword);
		row_print( 9, 38, "0x%08x", BSC.CS3WCR.lword);
		row_print(10, 38, "0x%08x", BSC.CS5AWCR.lword);

		row_print(1, 1, "Detected level: %s", level_string(level));
		row_print(2, 1, "dclear(): %d us", time_dclear);
		if(t >= 0)
			row_print(3, 1, "Timer: %.1D s", counter);
		else
			row_print(3, 1, "Timer: none available");

		fkey_button(1, "Set: F1");
		fkey_button(2, "Set: F2");
		fkey_button(3, "Set: F3");
		fkey_button(4, "Set: F4");
		fkey_button(5, "Set: F5");
		dupdate();

		key = getkey_opt(GETKEY_DEFAULT, &update).key;
		update = 0;

		if(key == KEY_F1) clock_set_speed(CLOCK_SPEED_F1);
		if(key == KEY_F2) clock_set_speed(CLOCK_SPEED_F2);
		if(key == KEY_F3) clock_set_speed(CLOCK_SPEED_F3);
		if(key == KEY_F4) clock_set_speed(CLOCK_SPEED_F4);
		if(key == KEY_F5) clock_set_speed(CLOCK_SPEED_F5);
	}

	if(t >= 0)
		timer_stop(t);
}

#endif
