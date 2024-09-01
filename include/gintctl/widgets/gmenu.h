#include <gintctl/util.h>
#include <gint/display.h>
#include <justui/jscrolledlist.h>

/* struct menuentry: Selectable list element */
struct menuentry {
    /* Terminator when name = NULL */
    char const *name;
    void (*function)(void);
    int flags;
};

enum {
    /* SH3-only */
    MENU_SH3_ONLY = 0x01,
    /* SH4-only */
    MENU_SH4_ONLY = 0x02,
    /* Category heading */
    MENU_CATEGORY = 0x04,
};

/* gmenu: Scrolled list showing entries in gintctl's main menu.
   This is a wrapper around a jscrolledlist. */
typedef jscrolledlist gmenu;

gmenu *gmenu_create(struct menuentry *entries, void *parent);
