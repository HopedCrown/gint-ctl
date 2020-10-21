#include <gint/display.h>
#include <gint/std/stdio.h>

#include <gintctl/plot.h>
#include <gintctl/util.h>

struct tick_info
{
	int count;

	int32_t value[64];
	uint16_t px_pos[64];
	int8_t primary[64];

	int max_width;
	int max_height;
};

static int graph_x(struct plot *s, int v)
{
	return s->graph.x + (s->graph.w-1) * (v-s->min_x) / (s->max_x-s->min_x);
}

static int graph_y(struct plot *s, int v)
{
	return s->graph.y + s->graph.h -
		(s->graph.h-1) * (v-s->min_y) / (s->max_y-s->min_y);
}

static void layout_ticks(struct plot *s, int xy, struct tick_info *i)
{
	struct plot_ticks *t = xy ? &s->ticks_y : &s->ticks_x;

	char str[256];
	char const *fmt = t->format ? t->format : "%d";

	i->count = 0;
	i->max_width = 0;
	i->max_height = 0;

	if(!t->multiples) return;

	int subdivs = t->subtick_divisions > 0 ? t->subtick_divisions : 1;

	/* Start at the minimum value and work towards the maximum one */
	int min = xy ? s->min_y : s->min_x;
	int max = xy ? s->max_y : s->max_x;

	int v = min + t->multiples - 1;
	v -= (v % t->multiples);

	while(v <= max && i->count < 64)
	{
		i->primary[i->count] = (v % (t->multiples * subdivs) == 0);
		i->value[i->count] = v;
		i->px_pos[i->count] = xy ? graph_y(s, v) : graph_x(s, v);
		(i->count)++;

		if(t->formatter) t->formatter(str, 256, v);
		else snprintf(str, 256, fmt, v);

		int w, h;
		dsize(str, NULL, &w, &h);

		if(w > i->max_width)  i->max_width  = w;
		if(h > i->max_height) i->max_height = h;

		v += t->multiples;
	}
}

void plot(struct plot *s)
{
	struct tick_info tx, ty;
	char str[256];
	int tick_w, tick_h;

	int32_t *data_x = s->data_x;
	int32_t *data_y = s->data_y;

	int axis_spacing = _(2,3);
	int tick_length = _(2,4);

	/* Determine the bounds of the rendering area */

	s->min_x = 0;
	s->max_x = 0;
	s->min_y = 0;
	s->max_y = 0;

	for(int i = 0; i < s->data_len; i++)
	{
		if(data_x[i] < s->min_x) s->min_x = data_x[i];
		if(data_x[i] > s->max_x) s->max_x = data_x[i];

		if(data_y[i] < s->min_y) s->min_y = data_y[i];
		if(data_y[i] > s->max_y) s->max_y = data_y[i];
	}

	if(s->min_x == s->max_x || s->min_y == s->max_y) return;

	/* Determine the number, position and size of ticks */

	font_t const *f = dfont(NULL);
	dfont(f);

	/* Start with vertical ticks */
	s->graph.y = s->area.y;
	s->graph.h = s->area.h - axis_spacing - f->line_height;

	layout_ticks(s, 1, &ty);

	/* Continue with horizontal ticks */
	s->graph.x = s->area.x + axis_spacing + ty.max_width;
	s->graph.w = s->area.w - (s->graph.x - s->area.x);

	layout_ticks(s, 0, &tx);

	/* Render grid, first secondary ticks, then primary ticks */

	int dotted = s->grid.dotted;

	for(int i = 0; i < ty.count; i++) if(!ty.primary[i])
	{
		int y = ty.px_pos[i];
		for(int x = s->graph.x; x < s->graph.x + s->graph.w; x++)
		{
			if(dotted && !((x^y) & 1)) continue;
			dpixel(x, y, s->grid.secondary_color);
		}
	}
	for(int i = 0; i < tx.count; i++) if(!tx.primary[i])
	{
		int x = tx.px_pos[i];
		for(int y = s->graph.y; y < s->graph.y + s->graph.h; y++)
		{
			if(dotted && !((x^y) & 1)) continue;
			dpixel(x, y, s->grid.secondary_color);
		}
	}
	for(int i = 0; i < ty.count; i++) if(ty.primary[i])
	{
		int y = ty.px_pos[i];
		for(int x = s->graph.x; x < s->graph.x + s->graph.w; x++)
		{
			if(dotted && !((x^y) & 1)) continue;
			dpixel(x, y, s->grid.primary_color);
		}
	}
	for(int i = 0; i < tx.count; i++) if(tx.primary[i])
	{
		int x = tx.px_pos[i];
		for(int y = s->graph.y; y < s->graph.y + s->graph.h; y++)
		{
			if(dotted && !((x^y) & 1)) continue;
			dpixel(x, y, s->grid.primary_color);
		}
	}

	/* Render ticks */

	int horz_axis = s->graph.y + s->graph.h;
	int vert_axis = s->graph.x;

	for(int i = 0; i < ty.count; i++)
	{
		char const *format = s->ticks_y.format;
		if(!format) format = "%d";

		if(s->ticks_y.formatter)
			s->ticks_y.formatter(str, 256, ty.value[i]);
		else snprintf(str, 256, format, ty.value[i]);
		dsize(str, NULL, NULL, &tick_h);

		/* Try to center the text left of the tick, but move it up or
		   down if it overflows from the render region */
		int y = ty.px_pos[i] - (tick_h >> 1);
		if(y < s->graph.y)
			y = s->graph.y;
		if(y + tick_h > s->graph.y + s->graph.h)
			y = s->graph.y + s->graph.h - tick_h;

		dline(vert_axis, ty.px_pos[i], vert_axis + tick_length,
			ty.px_pos[i], C_BLACK);

		if(!ty.primary[i]) continue;
		dtext_opt(vert_axis - axis_spacing, y, C_BLACK, C_NONE,
			DTEXT_RIGHT, DTEXT_TOP, str);
	}

	for(int i = 0; i < tx.count; i++)
	{
		char const *format = s->ticks_y.format;
		if(!format) format = "%d";

		if(s->ticks_x.formatter)
			s->ticks_x.formatter(str, 256, tx.value[i]);
		else snprintf(str, 256, format, tx.value[i]);
		dsize(str, NULL, &tick_w, NULL);

		/* Try to center the text below the tick, but move it left or
		   right if it overflows from the render region */
		int x = tx.px_pos[i] - ((tick_w+1) >> 1);
		if(x < s->graph.x)
			x = s->graph.x;
		if(x+tick_w > s->graph.x + s->graph.w)
			x = s->graph.x + s->graph.w - tick_w;

		dline(tx.px_pos[i], horz_axis, tx.px_pos[i],
			horz_axis - tick_length, C_BLACK);

		if(!tx.primary[i]) continue;
		dtext(x, horz_axis + axis_spacing, C_BLACK, str);
	}

	/* Render axes */

	int x2 = s->area.x + s->area.w - 1;
	dline(vert_axis, s->area.y, vert_axis, horz_axis, C_BLACK);
	dline(vert_axis, horz_axis, x2, horz_axis, C_BLACK);
	dline(vert_axis, s->area.y, x2, s->area.y, C_BLACK);
	dline(x2, s->area.y, x2, horz_axis, C_BLACK);

	/* Plot data */

	int last_x = 0;
	int last_y = 0;

	for(int i = 0; i < s->data_len; i++)
	{
		int x = graph_x(s, s->data_x[i]);
		int y = graph_y(s, s->data_y[i]);

		if(i > 0) dline(last_x, last_y, x, y, s->color);

		last_x = x;
		last_y = y;
	}
}
