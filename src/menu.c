#include <gint/defs/util.h>

#include <gintctl/menu.h>
#include <gintctl/util.h>

/* menu_init(): Initialize a menu list */
void menu_init(struct menu *menu, int visible)
{
	menu->len = 0;
	while(menu->entries[menu->len].name) menu->len++;

	menu->offset = 0;
	menu->pos = 0;
	menu->visible = visible;
}

/* menu_move(): Move the cursor in a menu */
void menu_move(struct menu *menu, int key, int wrap)
{
	int max_offset = max(menu->len - menu->visible, 0);

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
		if(menu->pos > menu->offset + menu->visible - 1
			&& menu->offset + 1 < max_offset)
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
	int i = 0;

	row_title(menu->name);

	while(i+1 <= menu->visible && items[offset+i].name)
	{
		row_print(i+1, 2, items[offset+i].name);
		i++;
	}

	if(offset > 0) row_right(1, "^");
	if(items[offset+i].name) row_right(row_count(), "v");

	int selected = pos - offset + 1;
	if(selected >= 1 && selected <= menu->visible) row_highlight(selected);
}

/* menu_exec(): Execute the currently-selected function of a menu */
void menu_exec(struct menu const *menu)
{
	void (*fun)(void) = menu->entries[menu->pos].function;
	if(fun) fun();
}
