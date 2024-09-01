#include <gintctl/widgets/gmenu.h>
#include <gintctl/util.h>
#include <gint/hardware.h>

static void gmenu_info(struct jlist *list, int index, jlist_item_info *info)
{
	struct menuentry const *entries = list->user, *entry = &entries[index];

	info->selectable = !(entry->flags & MENU_CATEGORY);
	info->triggerable = info->selectable;
	info->selection_style = JLIST_SELECTION_INVERT;
	info->natural_width = 100;
	info->natural_height = _(8,14);
}

static void gmenu_paint(int x, int y, GUNUSED int w, GUNUSED int h,
	struct jlist *list, int index, GUNUSED bool selected)
{
	struct menuentry const *entries = list->user, *entry = &entries[index];

	int dx = (entry->flags & MENU_CATEGORY) ? 0 : _(6, 8);
	dtext(x + _(1,3) + dx, y + _(0,2), C_BLACK, entry->name);
}

gmenu *gmenu_create(struct menuentry *entries, void *parent)
{
	jscrolledlist *sl = jscrolledlist_create(gmenu_info, gmenu_paint, parent);
	if(!sl)
		return NULL;

	/* Filter menu entries based on processor type */
	int len = 0;

	for(int i = 0; entries[i].name; i++)
	{
		int f = entries[i].flags;
		if(isSH3() && (f & MENU_SH4_ONLY)) continue;
		if(isSH4() && (f & MENU_SH3_ONLY)) continue;

		entries[len++] = entries[i];
	}

	jlist_update_model(sl->list, len, entries);
	jframe_set_visibility_margin(sl->frame, _(4,8), _(15,30));

	return sl;
}
