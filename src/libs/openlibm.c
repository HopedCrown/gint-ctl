#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/libs.h>
#include <gintctl/plot.h>

#define __BSD_VISIBLE 1
#include <openlibm/openlibm.h>

/* gintctl_libs_openlibm(): OpenLibm floating-point functions */
void gintctl_libs_openlibm(void)
{
	/* Sine curve magnified 1000 times because plot() only uses ints */
	int32_t x[128], y[128];

	for(int i = 0; i < 128; i++)
	{
		x[i] = 1000 * i * M_PI / 20;
		y[i] = 1000 * sin(i * M_PI / 20);
	}

	dclear(C_WHITE);

	#ifdef FXCG50
	row_title("OpenLibm floating-point functions");
	row_print(1, 1, "Basic sine curve:");

	struct plot plotspec = {
		.area = {
			.x = 20, .w = 354,
			.y = 40, .h = 80,
		},
		.data_x = x,
		.data_y = y,
		.data_len = 128,
		.color = C_RED,

		.ticks_x = {
			.multiples = 785, /* pi/4 */
			.subtick_divisions = 4,
			.format = "",
		},
		.ticks_y = {
			.multiples = 250, /* 0.25 */
			.subtick_divisions = 2,
			.format = "",
		},

		.grid = {
			.level = PLOT_FULLGRID,
			.primary_color = C_RGB(20, 20, 20),
			.secondary_color = C_RGB(28, 28, 28),
			.dotted = 1,
		},
	};
	plot(&plotspec);

	row_print(8, 1, "exp(1.0) = %.8j", (int)(1e8*exp(1.0)));
	row_print(9, 1, "atan(1.0)*4 = %.8j", (int)(1e8*4*atan(1.0)));
	#endif

	dupdate();
	while(getkey().key != KEY_EXIT) {}
}
