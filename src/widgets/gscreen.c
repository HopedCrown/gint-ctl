#include <gintctl/widgets/gscreen.h>
#include <gintctl/assets.h>
#include <gintctl/util.h>

#include <justui/jlabel.h>
#include <justui/jfkeys.h>
#include <justui/jwidget-api.h>

#include <stdlib.h>

J_DEFINE_WIDGET(gscreen, event, destroy)

gscreen *gscreen_create(char const *name,
	_(bopti_image_t const *img, char const *labels), void *parent)
{
	if(gscreen_type_id < 0)
		return NULL;

	gscreen *s = malloc(sizeof *s);
	if(!s) return NULL;

	jwidget_init(&s->widget, gscreen_type_id, parent);
	jwidget_set_focus_policy(s, J_FOCUS_POLICY_SCOPE);
	jwidget_set_stretch(s, 1, 1, false);

	s->tabs = NULL;
	s->tab_count = 0;

	jlabel *title = name ? jlabel_create(name, s) : NULL;
	jwidget *stack = jwidget_create(s);
	jfkeys *fkeys = _(img,labels) ? jfkeys_create(_(img,labels), s) : NULL;

	if((name && !title) || !stack || (_(img,labels) && !fkeys)) {
		jwidget_destroy(s);
		return NULL;
	}

	s->title = title;
	s->fkeys = fkeys;

	jlayout_set_vbox(s)->spacing = _(1,3);
	jlayout_set_stack(stack);

	if(title) {
		jwidget_set_background(title, C_BLACK);
		jlabel_set_text_color(title, C_WHITE);
		jlabel_set_font(title, _(&font_title, dfont_default()));
		jwidget_set_stretch(title, 1, 0, false);

		#if GINT_RENDER_MONO
		jwidget_set_padding(title, 1, 1, 0, 1);
		jwidget_set_margin(title, 0, 0, 1, 0);
		#endif

		#if GINT_RENDER_RGB
		jwidget_set_padding(title, 3, 6, 3, 6);
		#endif
	}

	#if GINT_RENDER_RGB
	jwidget_set_padding(stack, 1, 3, 1, 3);
	#endif

	jwidget_set_stretch(stack, 1, 1, false);
	return s;
}

bool gscreen_poly_event(void *s0, jevent e)
{
	gscreen *s = s0;

	/* Give the event to the fkeys if it wants it */
	return (s->fkeys && jwidget_event(s->fkeys, e))
		|| jwidget_poly_event(s, e);
}

void gscreen_poly_destroy(void *s0)
{
	gscreen *s = s0;
	free(s->tabs);
}

/* tab_stack(): Stacked widget where the tabs are located */
static jwidget *tab_stack(gscreen *s)
{
	int index = (s->title != NULL) ? 1 : 0;
	return s->widget.children[index];
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

	jwidget_add_child(tab_stack(s), widget);
	jwidget_set_stretch(widget, 1, 1, false);

	/* Set focus of gscreen's scope to this widget */
	if(s->tab_count == 1)
		jwidget_scope_set_target(s, focus);
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
	s->tabs[l->active].focus = jwidget_scope_get_target(s);
	jwidget_scope_set_target(s, s->tabs[tab].focus);

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
