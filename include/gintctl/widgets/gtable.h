//---
// gintctl.widgets.gtable: gintctl's scrolling tables
//---

#ifndef _GINTCTL_WIDGETS_GTABLE
#define _GINTCTL_WIDGETS_GTABLE

#include <justui/jwidget.h>
#include <gint/display.h>

/* gtable: A dynamic scrolling table

   This widget is a table with a header and a set of rows, that scrolls
   vertically to show more rows than there is space available on-screen.

   The columns all have their own width, label, and font. During rendering, the
   data is provided to the table through a user-provided function that
   generates strings for every column in a chosen row. */
typedef struct gtable {
	jwidget widget;
	/* Column details */
	struct gtable_column *meta;
	/* Function to generate the strings for a row */
	void (*generator)(struct gtable *g, int row, j_arg_t arg);
	j_arg_t arg;
	/* Number of columns, number of rows visible on-screen, number of rows,
	   and current top row */
	uint8_t columns;
	uint8_t visible;
	uint16_t rows;
	uint16_t offset;
	/* Row height */
	uint8_t row_height;
	/* Additional row spacing */
	uint8_t row_spacing;

	/* Coordinates during rendering */
	int16_t x, y;

} gtable;

/* gtable_create(): Create a scrolling table

   The number of columns of the table must be fixed, while the number of rows
   can be set and changed later with gtable_set_rows().

   The generator is a function that will be called during rendering to generate
   strings for each row. It should have the following prototype, where the last
   argument is an optional object of a type listed in j_arg_t (essentially an
   integer or a pointer).

     void generator(gtable *g, int row [, <an type of j_arg_t>])

   The generator should return all the strings for the row by a call to
   gtable_provide(). */
gtable *gtable_create(int columns, void *generator, j_arg_t arg, void *parent);

/* gtable_provide(): Pass strings to display on a given row

   The strings should be passed as variable arguments. A NULL terminator is
   added automatically. The strings will not be used after the call finishes,
   so they can be allocated statically or on the stack of the caller. */
void gtable_provide(gtable *g, ...);
#define gtable_provide(...) gtable_provide(__VA_ARGS__, NULL)

//---
// Configuration of columns
//---

/* gtable_set_column_title(): Set the title of a column */
void gtable_set_column_title(gtable *t, int column, char const *title);

/* gtable_set_column_font(): Set the font of a column */
void gtable_set_column_font(gtable *t, int column, font_t const *font);

/* gtable_set_column_size(): Set the size of a column
   This function sets the size of a column in relative units. During layout,
   the width of the table will be split, and each column will receive space
   proportional to their size setting. The default is 1 for all columns. */
void gtable_set_column_size(gtable *t, int column, uint size);

/* The previous functions have group-setting functions to set properties for
   all columns at once. Simply list the headers/fonts/widths all at once;
   columns 0 through the number of args will be assigned. */

void gtable_set_column_titles(gtable *t, ...);
#define gtable_set_column_titles(...) \
	gtable_set_column_titles(__VA_ARGS__, NULL)

void gtable_set_column_sizes(gtable *t, ...);
#define gtable_set_column_sizes(...) \
	gtable_set_column_sizes(__VA_ARGS__, 0)

void gtable_set_column_fonts(gtable *t, ...);
#define gtable_set_column_fonts(...) \
	gtable_set_column_fonts(__VA_ARGS__, NULL)

/* gtable_set_font(): Set the font for all columns */
void gtable_set_font(gtable *t, font_t const *font);

//---
// Configuration of rows
//---

/* gtable_set_rows(): Set the number of rows */
void gtable_set_rows(gtable *t, int rows);

/* gtable_set_row_height(): Fix row height instead of guessing from font
   Setting 0 will reset to the default behavior. */
void gtable_set_row_height(gtable *t, int row_height);

/* gtable_set_row_spacing(): Set additional row spacing */
void gtable_set_row_spacing(gtable *t, int row_spacing);

//---
// Movement
//---

/* gtable_scroll_to(): Scroll to the specified offset (if acceptable) */
void gtable_scroll_to(gtable *t, int offset);

/* gtable_end(): Offset of the end of the table */
int gtable_end(gtable *t);

#endif /* _GINTCTL_WIDGETS_GTABLE */
