//---
// gintctl.widgets.gscreen: gintctl's extended standard scene
//---

#ifndef _GINTCTL_WIDGETS_GSCREEN
#define _GINTCTL_WIDGETS_GSCREEN

#include <justui/jscene.h>
#include <justui/jlabel.h>
#include <justui/jfkeys.h>
#include <gintctl/util.h>
#include <gint/display.h>

struct gscreen_tab;

/* gscreen: gintctl's standard screen.

   The gscreen is a scene with a stylized title bar, an optional function key
   bar at the bottom, and a stacked layout of tabs in the middle. Each tab can
   have specific properties set, and the gscreen handles some of the focus
   management caused by hiding and showing a lot of widgets in tabs.

   The scene of a gscreen can be accessed with (the_gscreen->scene). */
typedef struct {
	jscene *scene;
	/* Number of tabs */
	int tab_count;
	/* Information on tabs */
	struct gscreen_tab *tabs;
	/* Fixed widets */
	jlabel *title;
	jfkeys *fkeys;

} gscreen;

struct gscreen_tab {
   /* Set whether the title is visible or the fkeys is visible */
	bool title_visible, fkeys_visible;
   /* Fkey level associated with the tab */
   int fkey_level;
	/* Most recent focused widget in the tab (regains focus when switching) */
	jwidget *focus;
};

/* gscreen_create(): Set up a standard screen.
   If (title = NULL), the screen has no title bar, if (fkeys = NULL) it has no
   function bar. To show a title/function bar on some tabs but not all, create
   one here and use gscreen_set_tab_{title,fkeys}_visible(). */

#if GINT_RENDER_MONO
gscreen *gscreen_create(char const *title, bopti_image_t const *fkeys);
#define gscreen_create2(short, img, long, fkeys) gscreen_create(short, img)
#endif

#if GINT_RENDER_RGB
gscreen *gscreen_create(char const *title, char const *fkeys);
#define gscreen_create2(short, img, long, fkeys) gscreen_create(long, fkeys)
#endif

void gscreen_destroy(gscreen *s);

//---
// Function bar settings
//---

/* gscreen_set_fkeys_level(): Override the function key bar */
void gscreen_set_fkeys_level(gscreen *s, int level);

//---
// Tab settings
//---

/* gscreen_add_tab(): Add a tab to the stacked layout
   The child's parent will be changed. The widget will also have a stretch of
   (1, 1, false) set by default. If not NULL, the last parameter indicates
   which widget to focus when the tab is first shown. */
void gscreen_add_tab(gscreen *s, void *widget, void *focus);

/* gcreen_add_tabs(): Add several tabs at once, with no special parameters */
void gscreen_add_tabs(gscreen *s, ...);
#define gscreen_add_tabs(...) gscreen_add_tabs(__VA_ARGS__, NULL)

/* gscreen_set_tab_title_visible(): Set whether title bar is shown on a tab */
void gscreen_set_tab_title_visible(gscreen *s, int tab, bool visible);

/* gscreen_set_tab_fkeys_visible(): Set whether fkeys are shown on a tab */
void gscreen_set_tab_fkeys_visible(gscreen *s, int tab, bool visible);

/* gscreen_set_tab_fkeys_level(): Set fkeys level shown on a tab */
void gscreen_set_tab_fkeys_level(gscreen *s, int tab, int level);

//---
// Tab navigation
//---

/* gscreen_show_tab(): Show a tab from the stack
   Returns true if the tab changed, false if it was already active or if it is
   an invalid tab widget. */
bool gscreen_show_tab(gscreen *s, int tab);

/* gscreen_current_tab(): Give the currently visible tab */
int gscreen_current_tab(gscreen *s);

/* gscreen_in(): Check if we're in a specific tab */
bool gscreen_in(gscreen *s, int tab);

//---
// Focus management
//---

/* Set focus for the current tab */
void gscreen_focus(gscreen *s, void *widget);

#endif /* _GINTCTL_WIDGETS_GSCREEN */
