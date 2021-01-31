#include <gint/defs/util.h>
#include <gint/hardware.h>

#include <gintctl/menu.h>
#include <gintctl/util.h>

/* menu_init(): Initialize a menu list */
void menu_init(struct menu *menu, int top, int bottom)
{
	menu->len = 0;

	for(int i = 0; menu->entries[i].name; i++)
	{
		int f = menu->entries[i].flags;
		if(isSH3() && (f & MENU_SH4_ONLY)) continue;
		if(isSH4() && (f & MENU_SH3_ONLY)) continue;

		menu->entries[menu->len++] = menu->entries[i];
	}

	menu->offset = 0;
	menu->pos = 0;
	menu->top = top + 1;
	menu->bottom = row_count() - bottom + 1;
}

/* menu_move(): Move the cursor in a menu */
void menu_move(struct menu *menu, int key, int wrap)
{
	int visible = menu->bottom - menu->top;
	int max_offset = max(menu->len - visible, 0);

	if(key == KEY_UP && menu->pos > 0)
	{
		menu->pos--;
		menu->offset = min(menu->offset, menu->pos);
	}
	else if(key == KEY_UP && !menu->pos && wrap)
	{
		menu->pos = menu->len - 1;
		menu->offset = max_offset;
	}

	if(key == KEY_DOWN && menu->pos + 1 < menu->len)
	{
		menu->pos++;
		if(menu->pos > menu->offset + visible - 1
			&& menu->offset + 1 <= max_offset)
		{
			menu->offset++;
		}
	}
	else if(key == KEY_DOWN && menu->pos + 1 == menu->len && wrap)
	{
		menu->pos = 0;
		menu->offset = 0;
	}
}

/* menu_show(): Render a list menu */
void menu_show(struct menu const *menu)
{
	struct menuentry const *items = menu->entries;
	int offset = menu->offset, pos = menu->pos;

	/* Min and max writable rows */
	int top = menu->top, bottom = menu->bottom;

	int i = 0, j = top;

	/* On fx9860g, only show the title if row is left for it */
	if(_(top > 0, 1)) row_title(menu->name);

	while(j < bottom && items[offset+i].name)
	{
		row_print(j, 2, items[offset+i].name);
		i++, j++;
	}

	if(menu->len > bottom - top)
	{
		scrollbar(offset, menu->len, top, bottom);
	}

	int selected = top + (pos - offset);
	if(selected >= top && selected < bottom) row_highlight(selected);
}

/* menu_exec(): Execute the currently-selected function of a menu */
void menu_exec(struct menu const *menu)
{
	void (*fun)(void) = menu->entries[menu->pos].function;
	if(fun) fun();
}
