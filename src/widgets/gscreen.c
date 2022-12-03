#include <gintctl/widgets/gscreen.h>
#include <gintctl/assets.h>
#include <gintctl/util.h>

#include <justui/jscene.h>
#include <justui/jlabel.h>
#include <justui/jfkeys.h>

#include <stdlib.h>

#ifdef FX9860G
gscreen *gscreen_create(char const *name, bopti_image_t const *img)
#endif
#ifdef FXCG50
gscreen *gscreen_create(char const *name, char const *labels)
#endif
{
	gscreen *g = malloc(sizeof *g);
	if(!g) return NULL;

	jscene *s = jscene_create_fullscreen(NULL);
	if(!s) { free(g); return NULL; }

	g->scene = s;
	g->tabs = NULL;
	g->tab_count = 0;

	jlabel *title = name ? jlabel_create(name, s) : NULL;
	jwidget *stack = jwidget_create(s);
	jfkeys *fkeys = _(img,labels) ? jfkeys_create(_(img,labels), s) : NULL;

	if((name && !title) || !stack || (_(img,labels) && !fkeys)) {
		jwidget_destroy(s);
		return NULL;
	}

	g->title = title;
	g->fkeys = fkeys;

	jlayout_set_vbox(s)->spacing = _(1,3);
	jlayout_set_stack(stack);

	if(title) {
		jwidget_set_background(title, C_BLACK);
		jlabel_set_text_color(title, C_WHITE);
		jlabel_set_font(title, _(&font_title, dfont_default()));
		jwidget_set_stretch(title, 1, 0, false);

		#ifdef FX9860G
		jwidget_set_padding(title, 1, 1, 0, 1);
		jwidget_set_margin(title, 0, 0, 1, 0);
		#endif

		#ifdef FXCG50
		jwidget_set_padding(title, 3, 6, 3, 6);
		#endif
	}

	#ifdef FXCG50
	jwidget_set_padding(stack, 1, 3, 1, 3);
	#endif

	jwidget_set_stretch(stack, 1, 1, false);
	return g;
}

void gscreen_destroy(gscreen *s)
{
	if(s->scene) jwidget_destroy(s->scene);
	free(s->tabs);
	free(s);
}

/* tab_stack(): Stacked widget where the tabs are located */
static jwidget *tab_stack(gscreen *s)
{
	int index = (s->title != NULL) ? 1 : 0;
	return s->scene->widget.children[index];
}

//---
// Function bar settings
//---

void gscreen_set_fkeys_level(gscreen *s, int level)
{
	jfkeys_set_level(s->fkeys, level);
}

//---
// Tab settings
//---

void gscreen_add_tab(gscreen *s, void *widget, void *focus)
{
	struct gscreen_tab *t = realloc(s->tabs, (s->tab_count+1) * sizeof *t);
	if(!t) return;

	s->tabs = t;
	s->tabs[s->tab_count].title_visible = true;
	s->tabs[s->tab_count].fkeys_visible = true;
	s->tabs[s->tab_count].focus = focus;
	s->tabs[s->tab_count].fkey_level = 0;
	s->tab_count++;

	if(s->tab_count == 1)
		jscene_set_focused_widget(s->scene, focus);

	jwidget_add_child(tab_stack(s), widget);
	jwidget_set_stretch(widget, 1, 1, false);
}

#undef gscreen_add_tabs
void gscreen_add_tabs(gscreen *s, ...)
{
	va_list args;
	va_start(args, s);
	jwidget *w;

	while((w = va_arg(args, jwidget *)))
		gscreen_add_tab(s, w, NULL);

	va_end(args);
}

void gscreen_set_tab_title_visible(gscreen *s, int tab, bool visible)
{
	if(!s->title || tab < 0 || tab >= s->tab_count) return;
	s->tabs[tab].title_visible = visible;

	if(gscreen_current_tab(s) == tab)
		jwidget_set_visible(s->title, visible);
}

void gscreen_set_tab_fkeys_visible(gscreen *s, int tab, bool visible)
{
	if(!s->fkeys || tab < 0 || tab >= s->tab_count) return;
	s->tabs[tab].fkeys_visible = visible;

	if(gscreen_current_tab(s) == tab)
		jwidget_set_visible(s->fkeys, visible);
}

void gscreen_set_tab_fkeys_level(gscreen *s, int tab, int level)
{
	if(!s->fkeys || tab < 0 || tab >= s->tab_count) return;
	s->tabs[tab].fkey_level = level;

	if(gscreen_current_tab(s) == tab)
		jfkeys_set_level(s->fkeys, level);
}

//---
// Tab navigation
//---

bool gscreen_show_tab(gscreen *s, int tab)
{
	jwidget *stack = tab_stack(s);
	jlayout_stack *l = jlayout_get_stack(stack);

	/* Find widget ID in the stack
	int i = 0;
	while(i < stack->child_count && stack->children[i] != widget) i++;
	if(i >= stack->child_count || l->active == i) return false; */
	if(tab < 0 || tab >= stack->child_count) return false;

	/* Update keyboard focus */
	s->tabs[l->active].focus = jscene_focused_widget(s->scene);
	jscene_set_focused_widget(s->scene, s->tabs[tab].focus);

	l->active = tab;
	stack->update = 1;

	/* Hide or show title and function key bar as needed */
	if(s->title) {
		jwidget_set_visible(s->title, s->tabs[tab].title_visible);
	}
	if(s->fkeys) {
		if(s->tabs[tab].fkeys_visible) {
			jfkeys_set_level(s->fkeys, s->tabs[tab].fkey_level);
			jwidget_set_visible(s->fkeys, true);
		}
		else {
			jwidget_set_visible(s->fkeys, false);
		}
	}

	return true;
}

int gscreen_current_tab(gscreen *s)
{
	jwidget *stack = tab_stack(s);
	jlayout_stack *l = jlayout_get_stack(stack);
	return l->active;
}

bool gscreen_in(gscreen *s, int tab)
{
	return gscreen_current_tab(s) == tab;
}

//---
// Focus management
//---

void gscreen_focus(gscreen *s, void *widget)
{
	return jscene_set_focused_widget(s->scene, widget);
}
