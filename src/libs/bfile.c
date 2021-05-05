#include <gint/gint.h>
#include <gint/bfile.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/std/stdio.h>

#include <gintctl/libs.h>
#include <gintctl/util.h>
#include <gintctl/assets.h>

#include <gintctl/widgets/gscreen.h>
#include <gintctl/widgets/gtable.h>

/* Names and file informations (allocated on the stack) */
static uint16_t (*test_names)[32][32];
static struct BFile_FileInfo *test_infos;

int explore_folder(uint16_t const *search, uint16_t names[32][32],
	struct BFile_FileInfo infos[32])
{
	/* Search descriptor */
	int sd = 0;
	/* Number of files found */
	int found = 0;

	int rc = BFile_FindFirst(search, &sd, names[found], &infos[found]);
	if(rc < 0) return 0;

	while(rc >= 0 && found < 32) {
		found++;
		rc = BFile_FindNext(sd, names[found], &infos[found]);
	}

	BFile_FindClose(sd);
	return found;
}

static void table_gen(gtable *t, int row)
{
	char c1[48]={0}, c2[16];

	for(int i = 0; i < 32 && (*test_names)[row][i]; i++)
		c1[i] = (*test_names)[row][i];

	sprintf(c2, "%d", test_infos[row].file_size);
	gtable_provide(t, c1, c2);
}

void gintctl_libs_bfile(void)
{
	/* Data for 32 files, each with 32 characters in the name */
	uint16_t names[32][32];
	struct BFile_FileInfo infos[32];

	test_names = &names;
	test_infos = infos;

	gtable *table = gtable_create(2, table_gen, NULL, NULL);
	gtable_set_rows(table, 0);
	gtable_set_column_titles(table, "Name", "Size");
	gtable_set_column_sizes(table, 3, 1);
	gtable_set_font(table, _(&font_mini, dfont_default()));
	jwidget_set_margin(table, 0, 2, 1, 2);

	gscreen *scr = gscreen_create2("BFile filesystem", &img_opt_libs_bfile,
		"BFile access to storage memory", "@LIST;;;;;");
	gscreen_add_tabs(scr, table, table);
	jscene_set_focused_widget(scr->scene, table);

	int key = 0;
	while(key != KEY_EXIT) {
		jevent e = jscene_run(scr->scene);

		if(e.type == JSCENE_PAINT) {
			dclear(C_WHITE);
			jscene_render(scr->scene);
			dupdate();
		}

		key = 0;
		if(e.type == JSCENE_KEY && e.key.type == KEYEV_DOWN)
			key = e.key.key;

		if(key == KEY_F1) {
			int rows = gint_world_switch(GINT_CALL(explore_folder,
				u"\\\\fls0\\*", (void *)names, (void *)infos));
			gtable_set_rows(table, rows);
		}
	}

	test_names = NULL;
	test_infos = NULL;
	gscreen_destroy(scr);
}
