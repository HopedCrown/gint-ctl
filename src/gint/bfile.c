#include <gint/config.h>
#include <gint/gint.h>
#include <gint/bfile.h>
#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>
#include <gintctl/assets.h>
#include <gintctl/ui.h>

#include <gintctl/widgets/gscreen.h>
#include <gintctl/widgets/gtable.h>

#include <stdio.h>
#include <stdlib.h>

#if !GINT_HW_CP

/* Names and file information */
static char **test_names = NULL;
static struct BFile_FileInfo *test_infos = NULL;

extern char *fc_to_utf8_alloc(uint16_t const *fc);

int explore_folder(uint16_t const *search)
{
	struct BFile_FileInfo info;
	int sd=-1, count=0, allocated=0;

	uint16_t *fc_path = malloc(512 * sizeof *fc_path);
	if(!fc_path) return 0;

	int rc = BFile_FindFirst(search, &sd, fc_path, &info);
	if(rc < 0) goto end;

	do {
		/* We want one extra space for the terminator in test_names[] */
		if(count+1 >= allocated) {
			allocated += 8;

			char **new_test_names = realloc(test_names,
				sizeof *test_names * allocated);
			struct BFile_FileInfo *new_test_infos = realloc(test_infos,
				sizeof *test_infos * allocated);

			if(new_test_names)
				test_names = new_test_names;
			if(new_test_infos)
				test_infos = new_test_infos;

			if(!new_test_names || !new_test_infos)
				goto end;
		}

		test_names[count] = fc_to_utf8_alloc(fc_path);
		test_names[count+1] = NULL;
		test_infos[count] = info;
		count++;

		rc = BFile_FindNext(sd, fc_path, &info);
	}
	while(rc >= 0);

end:
	if(sd >= 0)
		BFile_FindClose(sd);
	free(fc_path);
	return count;
}

static void table_gen(gtable *t, int row)
{
	char *c1 = test_names ? test_names[row] : "(null)";
	char c2[16];
	sprintf(c2, "%d", test_infos ? (int)test_infos[row].file_size : -1);
	gtable_provide(t, c1, c2);
}

void gintctl_gint_bfile(void)
{
	gscreen *s = gscreen_create2("BFile filesystem", &img_opt_gint_bfile,
		"BFile access to storage memory", "@LIST;;;;;", NULL);
	gintctl_scene_push(s);

	gtable *table = gtable_create(2, table_gen, NULL, NULL);
	gtable_set_rows(table, 0);
	gtable_set_column_titles(table, "Name", "Size");
	gtable_set_column_sizes(table, 3, 1);
	gtable_set_font(table, _(&font_mini, dfont_default()));
	jwidget_set_margin(table, 0, 2, 1, 2);
	gscreen_add_tab(s, table, table);

	while(1) {
		jevent e = jscene_run(gintctl_scene());

		if(jevent_is_press(e, KEY_EXIT))
			break;

		if(e.type == JFKEYS_TRIGGERED && e.data == 0) {
			if(test_names) {
				for(int i = 0; test_names[i]; i++)
					free(test_names[i]);
				free(test_names);
				test_names = NULL;
			}
			if(test_infos) {
				free(test_infos);
				test_infos = NULL;
			}

			int rows = gint_world_switch(GINT_CALL(explore_folder,
				u"\\\\fls0\\*"));
			gtable_set_rows(table, rows);
		}
	}
}

#endif /* GINT_HW_CP */
