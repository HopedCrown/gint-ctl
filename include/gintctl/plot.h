//---
//	gintctl:plot - Simple plotting utility
//---

#ifndef GINTCTL_PLOT
#define GINTCTL_PLOT

struct plot_ticks
{
	/* At most 64 ticks can be specified currently (easy to improve) */

	/* If set to non-zero, ticks are placed at multiples of this. By
	   default all of them are primary, use [subtick_divisions] to make
	   some ticks secondary */
	int multiples;
	/* No other way to specify ticks yet */

	/* Number of subtick per tick; subticks are marked but values are not
	   printed. Has no effect when 0 or 1 */
	int subtick_divisions;

	/* sprintf() format producting the text for ticks; defaults to "%d" */
	char const *format;
	/* Formatter function for specific needs; overrides [format] */
	void (*formatter)(char *str, size_t size, int32_t v);
};

struct plot
{
	/* Plot area, including axes and ticks */
	struct {
		int x, y, w, h;
	} area;

	/* Data points and color */
	int32_t *data_x;
	int32_t *data_y;
	int data_len;
	int color;

	/* Tick specification */
	struct plot_ticks ticks_x;
	struct plot_ticks ticks_y;

	/* Grid specification */
	struct {
		enum {
			PLOT_NOGRID = 0,
			PLOT_MAINGRID,
			PLOT_FULLGRID,
		} level;

		int primary_color;
		int secondary_color;
		int dotted;
	} grid;

	/** Internal parameters, computed by plot() **/

	/* Logical bounds */
	int min_x, max_x;
	int min_y, max_y;

	/* Concrete bounds */
	struct {
		int x, y, w, h;
	} graph;
};

/* plot(): Render a graph */
void plot(struct plot *plot);

#endif /* GINTCTL_PLOT */
