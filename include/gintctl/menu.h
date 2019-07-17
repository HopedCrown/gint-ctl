//---
//	gintctl:menu - Interactive list menus
//---

#ifndef GINTCTL_MENU
#define GINTCTL_MENU

#include <stddef.h>
#include <gint/keycodes.h>

/* struct menuentry: Selectable list element */
struct menuentry {
	char const *name;
	void (*function)(void);
};

struct menu {
	char const *name;

	int len;
	int offset;
	int pos;
	int visible;

	struct menuentry entries[];
};

/* menu_init(): Initialize a menu list
   @menu     Any list menu, even unitialized
   @visible  Number of lines that can be shown simultaneously */
void menu_init(struct menu *menu, int visible);

/* menu_move(): Move the cursor in a menu
   @menu  Initialized list menu
   @key   Either KEY_UP, indicating up, or KEY_DOWN indicating down
   @wrap  Allow wrap-around */
void menu_move(struct menu *menu, int key, int wrap);

/* menu_show(): Render a list menu */
void menu_show(struct menu const *menu);

/* menu_exec(): Execute the currently-selected function of a menu */
void menu_exec(struct menu const *menu);

#endif /* GINTCTL_MENU */
