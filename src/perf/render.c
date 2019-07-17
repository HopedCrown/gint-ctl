#include <gint/keyboard.h>
#include <gint/display.h>

#include <gintctl/util.h>
#include <gintctl/prof-contexts.h>
#include <gintctl/perf.h>

#include <libprof.h>

struct elapsed {
	uint32_t clear;
	uint32_t update;
	uint32_t rect1, rect2, rect3;
};

static void run_test(struct elapsed *time)
{
	#define test(ctx, out, command) {		\
		prof_clear(ctx);			\
		prof_enter(ctx);			\
		command;				\
		prof_leave(ctx);			\
		out = prof_time(ctx);			\
	}

	test(PROFCTX_DUPDATE, time->update,
		dupdate()
	);
	test(PROFCTX_DCLEAR,  time->clear,
		dclear(C_WHITE)
	);
	test(PROFCTX_DRECT1, time->rect1,
		drect(0, 0, 31, 31, C_WHITE)
	);
	test(PROFCTX_DRECT2, time->rect2,
		drect(1, 1, 32, 32, C_WHITE)
	);
	test(PROFCTX_DRECT3, time->rect3,
		drect(0, 0, 395, 223, C_WHITE)
	);

	#undef test
}

/* gintctl_perf_render(): Profile the display primitives */
void gintctl_perf_render(void)
{
	int key = 0, test = 0;
	struct elapsed time = {};

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);
		row_title("Rendering primitives");

		#ifdef FX9860G
		#warning gintctl_perf_render not implemented on fx9860g
		#endif

		#ifdef FXCG50
		row_print(1, 1, "This program measures the execution time of");
		row_print(2, 1, "common drawing functions.");

		row_print(4, 1, "Press F1 to start the test.");

		if(test)
		{
			print(6, 90, "dclear:");
			print(6, 105, "%.1j ms", time.clear / 100);
			print(6, 120, "%05x us", time.clear);

			print(83, 90, "dupdate:");
			print(83, 105, "%.1j ms", time.update / 100);
			print(83, 120, "%05x us", time.update);

			print(160, 90, "rect1:");
			print(160, 105, "%.1j ms", time.rect1 / 100);
			print(160, 120, "%05x us", time.rect1);

			print(237, 90, "rect2:");
			print(237, 105, "%.1j ms", time.rect2 / 100);
			print(237, 120, "%05x us", time.rect2);

			print(314, 90, "rect3:");
			print(314, 105, "%.1j ms", time.rect3 / 100);
			print(314, 120, "%05x us", time.rect3);
		}

		fkey_button(1, "START");
		#endif

		dupdate();
		key = getkey().key;

		if(key == KEY_F1) run_test(&time), test = 1;
	}
}
