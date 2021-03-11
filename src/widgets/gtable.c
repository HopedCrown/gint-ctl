#include <gintctl/widgets/gtable.h>
#include <justui/jwidget-api.h>
#include <gint/std/stdlib.h>
#include <gintctl/util.h>
#include <stdarg.h>

struct gtable_column {
	char const *title;
	font_t const *font;
	uint16_t size;
	uint16_t width;
};

/* Type identifier for gtable */
static int gtable_type_id = -1;

void update_visible(gtable *t);

gtable *gtable_create(int columns, void *generator, j_arg_t arg, void *parent)
{
	if(gtable_type_id < 0) return NULL;

	gtable *t = malloc(sizeof *t);
	if(!t) return NULL;

	t->meta = malloc(columns * sizeof *t->meta);
	if(!t->meta) { free(t); return NULL; }

	jwidget_init(&t->widget, gtable_type_id, parent);

	t->generator = generator;
	t->arg = arg;

	t->columns = columns;
	t->rows = 0;
	t->offset = 0;
	t->visible = 0;

	t->row_height = 0;
	t->row_spacing = 1;

	for(uint i = 0; i < t->columns; i++) {
		t->meta[i].title = NULL;
		t->meta[i].font = dfont_default();
		t->meta[i].width = 1;
	}

	return t;
}

//---
// Configuration of columns
//---

void gtable_set_column_title(gtable *t, int column, char const *title)
{
	if(column < 0 || column >= t->columns) return;
	t->meta[column].title = title;
	t->widget.update = 1;
}

void gtable_set_column_font(gtable *t, int column, font_t const *font)
{
	if(column < 0 || column >= t->columns) return;
	t->meta[column].font = (font ? font : dfont_default());
	t->widget.update = 1;
}

void gtable_set_column_size(gtable *t, int column, uint size)
{
	if(column < 0 || column >= t->columns) return;
	t->meta[column].size = size;
	t->widget.dirty = 1;
}

#undef gtable_set_column_titles
void gtable_set_column_titles(gtable *t, ...)
{
	va_list args;
	va_start(args, t);

	for(uint i = 0; i < t->columns; i++)
	{
		char const *title = va_arg(args, char const *);
		if(!title) break;
		gtable_set_column_title(t, i, title);
	}

	va_end(args);
}

#undef gtable_set_column_fonts
void gtable_set_column_fonts(gtable *t, ...)
{
	va_list args;
	va_start(args, t);

	for(uint i = 0; i < t->columns; i++)
	{
		font_t const *font = va_arg(args, font_t const *);
		if(!font) break;
		gtable_set_column_font(t, i, font);
	}

	va_end(args);
}

#undef gtable_set_column_sizes
void gtable_set_column_sizes(gtable *t, ...)
{
	va_list args;
	va_start(args, t);

	for(uint i = 0; i < t->columns; i++)
	{
		uint size = va_arg(args, uint);
		if(!size) break;
		gtable_set_column_size(t, i, size);
	}

	va_end(args);
}

void gtable_set_font(gtable *t, font_t const *font)
{
	for(uint i = 0; i < t->columns; i++) {
		gtable_set_column_font(t, i, font);
	}
}

//---
// Configuration of rows
//---

void gtable_set_rows(gtable *t, int rows)
{
	/* The re-layout will make sure we stay in the visible range */
	t->rows = max(rows, 0);
	t->widget.dirty = 1;
}

void gtable_set_row_height(gtable *t, int row_height)
{
	t->row_height = row_height;
	t->visible = 0;
	t->widget.dirty = 1;
}

void gtable_set_row_spacing(gtable *t, int row_spacing)
{
	t->row_spacing = row_spacing;
	t->visible = 0;
	t->widget.dirty = 1;
}

//---
// Movement
//---

/* Guarantee that the current positioning is valid */
static void bound(gtable *t)
{
	t->offset = min(t->offset, gtable_end(t));
}

void gtable_scroll_to(gtable *t, int offset)
{
	t->offset = offset;
	bound(t);
}

int gtable_end(gtable *t)
{
	update_visible(t);
	return max((int)t->rows - (int)t->visible, 0);
}

//---
// Polymorphic widget operations
//---

int compute_row_height(gtable *t)
{
	int row_height = t->row_height;
	if(row_height == 0) {
		for(int i = 0; i < t->columns; i++) {
			row_height = max(row_height, t->meta[i].font->line_height);
		}
	}
	return row_height;
}

/* Recompute (visible) based on the current size */
void update_visible(gtable *t)
{
	if(t->visible == 0) {
		int row_height = compute_row_height(t);
		int header_height = row_height + t->row_spacing + 1;

		int space = jwidget_content_height(t) - header_height;
		t->visible = max(0, space / (row_height + t->row_spacing));
	}
}

static void gtable_poly_csize(void *t0)
{
	gtable *t = t0;
	t->widget.w = 64;
	t->widget.h = 32;
}

static void gtable_poly_layout(void *t0)
{
	gtable *t = t0;
	t->visible = 0;
	update_visible(t);
	bound(t);

	int cw = jwidget_content_width(t);
	/* Leave space for a scrollbar */
	if(t->visible < t->rows) cw -= _(2,4);

	/* Compute the width of each column */
	uint total_size = 0;
	for(uint i = 0; i < t->columns; i++) {
		total_size += t->meta[i].size;
	}

	uint space_left = cw;
	for(uint i = 0; i < t->columns; i++) {
		t->meta[i].width = (t->meta[i].size * cw) / total_size;
		space_left -= t->meta[i].width;
	}

	/* Grant space left to the first columns */
	for(uint i = 0; i < t->columns && i < space_left; i++) {
		t->meta[i].width++;
	}
}

static void gtable_poly_render(void *t0, int base_x, int base_y)
{
	gtable *t = t0;
	int row_height = compute_row_height(t);
	int cw = jwidget_content_width(t);
	int y = base_y;

	for(uint i=0, x=base_x; i < t->columns; i++) {
		font_t const *old_font = dfont(t->meta[i].font);
		dtext(x, y, C_BLACK, t->meta[i].title);
		dfont(old_font);

		x += t->meta[i].width;
	}

	y += row_height + t->row_spacing;
	dline(base_x, y, base_x + cw - 1, y, C_BLACK);
	y += t->row_spacing + 1;
	int rows_y = y;

	for(uint i = 0; t->offset + i < t->rows && i < t->visible; i++) {
		t->x = base_x;
		t->y = y;

		t->generator(t, t->offset + i, t->arg);
		y += row_height + t->row_spacing;
	}

	/* Scrollbar */
	if(t->visible < t->rows) {
		/* Area where the scroll bar lives */
		int area_w = _(1,2);
		int area_x = base_x + cw - area_w;
		int area_y = rows_y;
		int area_h = jwidget_content_height(t) - (area_y - base_y);

		/* Position and size of scroll bar within the area */
		int bar_y = (t->offset  * area_h + t->rows/2) / t->rows;
		int bar_h = (t->visible * area_h + t->rows/2) / t->rows;

		drect(area_x, area_y + bar_y, area_x + area_w - 1,
			area_y + bar_y + bar_h, C_BLACK);
	}
}

#undef gtable_provide
void gtable_provide(gtable *t, ...)
{
	va_list args;
	va_start(args, t);

	for(uint i = 0; i < t->columns; i++) {
		char const *str = va_arg(args, char const *);
		if(!str) break;

		font_t const *old_font = dfont(t->meta[i].font);
		dtext(t->x, t->y, C_BLACK, str);
		dfont(old_font);

		t->x += t->meta[i].width;
	}

	va_end(args);
}

static bool gtable_poly_event(void *t0, jevent e)
{
	gtable *t = t0;
	int end = gtable_end(t);
	update_visible(t);

	if(e.type != JWIDGET_KEY) return false;

	key_event_t kev = e.data.key;
	if(kev.type == KEYEV_UP) return false;

	if(kev.key == KEY_DOWN && t->offset < end) {
		if(kev.shift) t->offset = end;
		else t->offset++;
		t->widget.update = 1;
		return true;
	}
	if(kev.key == KEY_UP && t->offset > 0) {
		if(kev.shift) t->offset = 0;
		else t->offset--;
		t->widget.update = 1;
		return true;
	}

	return false;
}

static void gtable_poly_destroy(void *t0)
{
	gtable *t = t0;
	free(t->meta);
}

/* gtable type definition */
static jwidget_poly type_gtable = {
	.name    = "gtable",
	.csize   = gtable_poly_csize,
	.layout  = gtable_poly_layout,
	.render  = gtable_poly_render,
	.event   = gtable_poly_event,
	.destroy = gtable_poly_destroy,
};

/* Type registration */
__attribute__((constructor(2000)))
static void j_register_gtable(void)
{
	gtable_type_id = j_register_widget(&type_gtable, "jwidget");
}
