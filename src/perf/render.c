#include <gint/keyboard.h>
#include <gint/display.h>

#include <gintctl/util.h>
#include <gintctl/perf.h>

#include <libprof.h>

#include <stdio.h>

struct elapsed {
	uint32_t clear;
	uint32_t update;
	uint32_t rect1, rect2, rect3;
	uint32_t fs_r5g6b5;
};

static void run_test(struct elapsed *time)
{
	time->clear = prof_exec({
		dclear(C_WHITE);
	});
	time->rect1 = prof_exec({
		drect(0, 0, 31, 31, C_WHITE);
	});
	time->rect2 = prof_exec({
		drect(1, 1, 32, 32, C_WHITE);
	});
	time->rect3 = prof_exec({
		drect(0, 0, _(127,395), _(63,223), C_WHITE);
	});

	#if GINT_RENDER_RGB
	extern bopti_image_t img_swift;
	time->fs_r5g6b5 = prof_exec({
		dimage(0, 0, &img_swift);
	});
	#endif
}

char *printtime(uint32_t us)
{
	static char str[20];

	if(us < 1000) sprintf(str, "%d us", us);
	else sprintf(str, "%.1D ms", us / 100);

	return str;
}

/* gintctl_perf_render(): Profile the display primitives */
void gintctl_perf_render(void)
{
	int key = 0, test = 0;
	struct elapsed time = {};

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		#if GINT_RENDER_MONO
		row_print(1, 1, "Rendering functions");

		if(test)
		{
			row_print(2, 1, "dclear:  %s", printtime(time.clear));
			row_print(3, 1, "dupdate: %s", printtime(time.update));
			row_print(4, 1, "rect1:   %s", printtime(time.rect1));
			row_print(5, 1, "rect2:   %s", printtime(time.rect2));
			row_print(6, 1, "rect3:   %s", printtime(time.rect3));
		}

		extern bopti_image_t img_opt_perf_render;
		dimage(0, 56, &img_opt_perf_render);
		#endif

		#if GINT_RENDER_RGB
		row_title("Rendering functions");
		row_print(1, 1, "This program measures the execution time of");
		row_print(2, 1, "common drawing functions.");

		row_print(4, 1, "Press F1 to start the test.");

		row_print(6,  2, "dclear():");
		row_print(7,  2, "dupdate():");
		row_print(8,  2, "drect() 32x32 (even position):");
		row_print(9,  2, "drect() 32x32 (odd position):");
		row_print(10, 2, "drect() 396x224:");
		row_print(11, 2, "dimage() 396x224 (p4):");

		if(test)
		{
			row_print(6,  35, "%s", printtime(time.clear));
			row_print(7,  35, "%s", printtime(time.update));
			row_print(8,  35, "%s", printtime(time.rect1));
			row_print(9,  35, "%s", printtime(time.rect2));
			row_print(10, 35, "%s", printtime(time.rect3));
			row_print(11, 35, "%s", printtime(time.fs_r5g6b5));
		}

		fkey_button(1, "START");
		#endif

		/* Make the test here as we don't want to re-update the screen.
		   Because of triple buffering this would display an old
		   frame such as the application's main menu. */
		time.update = prof_exec({
			dupdate();
		});
		key = getkey().key;

		if(key == KEY_F1) run_test(&time), test = 1;
	}
}
