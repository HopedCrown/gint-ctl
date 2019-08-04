#include <gint/display.h>
#include <gint/keyboard.h>
#include <gintctl/gint.h>

/* gintctl_gint_bopti(): Test image rendering */
void gintctl_gint_bopti(void)
{
#ifdef FXCG50
	extern image_t img_swift;
	extern image_t img_swords;

	int x = 396 - img_swords.width - 16;
	int y = 16;

	int key = 0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);
		bopti_render_noclip(0, 0, &img_swift, 0, 0, 396, 224);
		bopti_render_clip(x, y, &img_swords, 0, 0, img_swords.width,
			img_swords.height);
		dupdate();

		key = getkey().key;

		if(key == KEY_UP)    y--;
		if(key == KEY_DOWN)  y++;
		if(key == KEY_LEFT)  x--;
		if(key == KEY_RIGHT) x++;
	}
#endif
}
