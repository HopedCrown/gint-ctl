#include <gint/keyboard.h>
#include <gint/display.h>
#include <gint/std/stdio.h>

#include <gintctl/util.h>
#include <gintctl/prof-contexts.h>
#include <gintctl/perf.h>

#include <libprof.h>

struct elapsed {
	uint32_t clear;
	uint32_t update;
	uint32_t rect1, rect2, rect3;
	uint32_t fs_r5g6b5;
};

static void run_test(struct elapsed *time)
{
	#define test(out, command) {			\
		prof_clear(PROFCTX_RENDER);		\
		prof_enter(PROFCTX_RENDER);		\
		command;				\
		prof_leave(PROFCTX_RENDER);		\
		out = prof_time(PROFCTX_RENDER);	\
	}

	extern image_t img_swift;

	test(time->update,
		dupdate()
	);
	test(time->clear,
		dclear(C_WHITE)
	);
	test(time->rect1,
		drect(0, 0, 31, 31, C_WHITE)
	);
	test(time->rect2,
		drect(1, 1, 32, 32, C_WHITE)
	);
	test(time->rect3,
		drect(0, 0, _(127,395), _(63,223), C_WHITE)
	);

	#ifdef FXCG50
	test(time->fs_r5g6b5,
		dimage(0, 0, &img_swift)
	);
	#endif

	#undef test
}

char *printtime(uint32_t us)
{
	static char str[20];

	if(us < 1000) sprintf(str, "%d us", us);
	else sprintf(str, "%.1j ms", us / 100);

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

		#ifdef FX9860G
		row_print(1, 1, "Rendering functions");

		if(test)
		{
			row_print(2, 1, "dclear:  %s", printtime(time.clear));
			row_print(3, 1, "dupdate: %s", printtime(time.update));
			row_print(4, 1, "rect1:   %s", printtime(time.rect1));
			row_print(5, 1, "rect2:   %s", printtime(time.rect2));
			row_print(6, 1, "rect3:   %s", printtime(time.rect3));
		}

		extern image_t img_opt_perf_render;
		dimage(0, 56, &img_opt_perf_render);
		#endif

		#ifdef FXCG50
		row_title("Rendering functions");
		row_print(1, 1, "This program measures the execution time of");
		row_print(2, 1, "common drawing functions.");

		row_print(4, 1, "Press F1 to start the test.");

		if(test)
		{
			print(6, 90, "dclear:");
			print(6, 105, "%s", printtime(time.clear));

			print(83, 90, "dupdate:");
			print(83, 105, "%s", printtime(time.update));

			print(160, 90, "rect1:");
			print(160, 105, "%s", printtime(time.rect1));

			print(237, 90, "rect2:");
			print(237, 105, "%s", printtime(time.rect2));

			print(314, 90, "rect3:");
			print(314, 105, "%s", printtime(time.rect3));

			print(6, 130, "fullscreen img:");
			print(6, 145, "%s", printtime(time.fs_r5g6b5));
		}

		fkey_button(1, "START");
		#endif

		dupdate();
		key = getkey().key;

		if(key == KEY_F1) run_test(&time), test = 1;
	}
}
