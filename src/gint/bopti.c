#define GINT_NEED_VRAM
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
		dimage(0, 0, &img_swift);
		dimage(x, y, &img_swords);
		dupdate();

		key = getkey().key;

		if(key == KEY_UP)    y--;
		if(key == KEY_DOWN)  y++;
		if(key == KEY_LEFT)  x--;
		if(key == KEY_RIGHT) x++;
	}
#endif
}
