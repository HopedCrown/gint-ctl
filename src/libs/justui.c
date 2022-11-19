#include <gint/keyboard.h>
#include <gint/display.h>

#include <gintctl/libs.h>
#include <gintctl/widgets/gscreen.h>
#include <gintctl/widgets/gtable.h>
#include <gintctl/util.h>
#include <gintctl/assets.h>

#include <justui/jscene.h>
#include <justui/jwidget.h>
#include <justui/jlabel.h>
#include <justui/jinput.h>
#include <justui/jpainted.h>
#include <justui/jfkeys.h>

#include <stdio.h>

static int recursive_widget_count(void *w0)
{
	J_CAST(w)

	int total = 1;
	for(int k = 0; k < w->child_count; k++)
		total += recursive_widget_count(w->children[k]);

	return total;
}

static int nth_child(void *w0, int n, int l, jwidget **result, int *level)
{
	J_CAST(w)
	if(n-- == 0)
	{
		*result = w;
		*level = l;
		return n;
	}

	for(int k = 0; k < w->child_count && n >= 0; k++)
		n = nth_child(w->children[k], n, l+1, result, level);

	return n;
}

static void widget_tree_gen(gtable *t, int row, jwidget *root)
{
	jwidget *w = NULL;
	int indent;
	nth_child(root, row, 0, &w, &indent);
	if(!w) return;

	char c1[32], c2[16], c3[16];
	for(int i = 0; i < indent; i++) c1[i] = ' ';
	sprintf(c1 + indent, "%s", jwidget_type(w));
	sprintf(c2, "%dx%d", jwidget_full_width(w), jwidget_full_height(w));
	sprintf(c3, "%dx%d", jwidget_content_width(w), jwidget_content_height(w));

	gtable_provide(t, c1, c2, c3);
}

static void paint_pattern(int x, int y)
{
	for(int dx = 0; dx < 12; dx++)
	for(int dy = 0; dy < 12; dy++)
	{
		if(((x + dx) ^ (y + dy)) & 1) dpixel(x + dx, y + dy, C_BLACK);
	}
}

static void table_gen(gtable *t, int row)
{
	char c1[16], c2[16], c3[16];
	sprintf(c1, "%d:1", row);
	sprintf(c2, "%d:2", row);
	sprintf(c3, "%d:%d", row, t->visible);
	gtable_provide(t, c1, c2, c3);
}

/* gintctl_libs_justui(): Just User Interfaces */
void gintctl_libs_justui(void)
{
	gscreen *scr = gscreen_create2("JustUI Widgets", &img_opt_libs_jui,
		"JustUI graphical interfaces", "/SCENE;/TREE;#INPUT;;;");

	// Sample GUI

	jwidget *tab1 = jwidget_create(NULL);
	jlayout_set_vbox(tab1)->spacing = _(1,2);

	jwidget *c = jwidget_create(tab1);
	jlabel *c1 = jlabel_create("", c);
	jpainted_create(paint_pattern, NULL, 12, 12, c);
	gtable *c3 = gtable_create(3, table_gen, NULL, c);

	jwidget_set_border(c, J_BORDER_SOLID, 1, C_BLACK);
	jwidget_set_padding(c, 1, 1, 1, 1);
	jwidget_set_stretch(c, 1, 1, false);
	jlayout_set_hbox(c)->spacing = _(1,2);

	jlabel_set_font(c1, _(&font_uf5x7, dfont_default()));
	jlabel_set_alignment(c1, J_ALIGN_CENTER);
	jlabel_asprintf(c1, "Test\nlαbel\nxy=%d,%d", 7, 8);
	jwidget_set_border(c1, J_BORDER_SOLID, 1, C_BLACK);
	jwidget_set_stretch(c1, 1, 1, false);

	jwidget_set_stretch(c3, 2, 1, false);
	gtable_set_rows(c3, 5);
	gtable_set_column_titles(c3, "C1", "C2", "Column 3");
	gtable_set_column_sizes(c3, 1, 1, 3);
	gtable_set_font(c3, _(&font_mini, dfont_default()));

	jinput *input = jinput_create("Prompt:" _(," "), 12, tab1);
	jwidget_set_stretch(input, 1, 0, false);
	jinput_set_font(input, _(&font_uf5x7, dfont_default()));

	// Widget tree visualisation

	gtable *tree = gtable_create(3, widget_tree_gen, scr->scene, NULL);
	gtable_set_column_titles(tree, "Type", "Size", "Content");
	gtable_set_column_sizes(tree, 3, 2, 2);
	gtable_set_row_spacing(tree, _(2,3));
	gtable_set_font(tree, _(&font_mini, dfont_default()));

	// Scene setup

	gscreen_add_tab(scr, tab1, c3);
	gscreen_add_tab(scr, tree, tree);
	jscene_set_focused_widget(scr->scene, c3);
	gtable_set_rows(tree, recursive_widget_count(scr->scene));
	gscreen_set_tab_title_visible(scr, 1, _(false,true));

	int key = 0;
	while(key != KEY_EXIT)
	{
		jevent e = jscene_run(scr->scene);

		if(e.type == JSCENE_PAINT)
		{
			dclear(C_WHITE);
			jscene_render(scr->scene);
			dupdate();
		}

		if(e.type == JINPUT_VALIDATED && e.source == input)
		{
			gscreen_focus(scr, c3);
			jlabel_snprintf(c1, 20, "New!\n%s", jinput_value(input));
		}
		if(e.type == JINPUT_CANCELED && e.source == input)
		{
			gscreen_focus(scr, c3);
		}

		if(e.type != JSCENE_KEY || e.key.type != KEYEV_DOWN) continue;
		key = e.key.key;

		if(key == KEY_F3 && gscreen_in(scr, 0))
		{
			bool input_focused = (jscene_focused_widget(scr->scene) == input);
			gscreen_focus(scr, input_focused ? (void *)c3 : (void *)input);
		}

		if(key == KEY_F1) gscreen_show_tab(scr, 0);
		if(key == KEY_F2) gscreen_show_tab(scr, 1);

		#ifdef FX9860G
		if(key == KEY_F6) screen_mono(u"\\\\fls0\\justui.bin");
		#endif
	}

	gscreen_destroy(scr);
}
