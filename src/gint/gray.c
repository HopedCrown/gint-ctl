#ifdef FX9860G

#include <gint/gray.h>
#include <gint/keyboard.h>
#include <gint/std/string.h>
#include <gint/std/stdio.h>
#include <gintctl/gint.h>

/* gintctl_gint_gray(): Gray engine tuning */
void gintctl_gint_gray(void)
{
	uint32_t delays[2];
	gray_config(&delays[0], &delays[1]);

	int key = 0, sel = 0;
	char str[20];

	gray_start();

	while(key != KEY_EXIT)
	{
		gclear(C_WHITE);

		gtext(1, 0, "Gray engine tuning", C_BLACK, C_NONE);

		sprintf(str, "Light%5u", delays[0]);
		gtext(13, 24, str, C_BLACK, C_NONE);
		sprintf(str, "Dark %5u", delays[1]);
		gtext(13, 32, str, C_BLACK, C_NONE);

		int y = 24 + (sel << 3);
		gtext(7, y, "<", C_BLACK, C_NONE);
		gtext(73, y, ">", C_BLACK, C_NONE);

		grect(96, 16, 127, 31, C_LIGHT);
		grect(96, 32, 127, 47, C_DARK);

		extern bopti_image_t img_opt_gint_gray;
		gimage(0, 56, &img_opt_gint_gray);

		gupdate();
		key = getkey().key;

		if(key == KEY_UP && sel) sel = 0;
		if(key == KEY_DOWN && !sel) sel = 1;

		if(key == KEY_LEFT)
			delays[sel]--;
		else if(key == KEY_RIGHT)
			delays[sel]++;
		else if(key == KEY_F1)
			delays[0] = 680, delays[1] = 1078;
		else if(key == KEY_F2)
			delays[0] = 762, delays[1] = 1311;
		else if(key == KEY_F3)
			delays[0] = 869, delays[1] = 1097;
		else if(key == KEY_F4)
			delays[0] = 869, delays[1] = 1311;
		else if(key == KEY_F5)
			delays[0] = 937, delays[1] = 1425;
		else continue;

		if(delays[sel] < 100) delays[sel] = 100;
		if(delays[sel] > 3000) delays[sel] = 3000;
		gray_delays(delays[0], delays[1]);
	}
	gray_stop();
}

/* gintctl_gint_grayrender(): Gray rendering functions */
void gintctl_gint_grayrender(void)
{
	int x, y;

	gray_start();
	gclear(C_WHITE);

	gtext(1, 1, "Gray rendering", C_BLACK, C_NONE);

	x = 6, y = 12;
	grect(x,      y,      x + 15, y + 15, C_WHITE);
	grect(x + 16, y,      x + 31, y + 15, C_LIGHT);
	grect(x + 32, y,      x + 47, y + 15, C_DARK);
	grect(x + 48, y,      x + 63, y + 15, C_BLACK);
	grect(x,      y,      x + 63, y +  3, C_LIGHTEN);
	grect(x,      y + 12, x + 63, y + 15, C_DARKEN);

	x = 104, y = 0;
	grect(x, y, x + 23, y + 32, C_BLACK);
	gtext(x - 13, y + 1,  "White", C_WHITE, C_NONE);
	gtext(x - 13, y + 9,  "Light", C_LIGHT, C_NONE);
	gtext(x - 13, y + 17, "Dark",  C_DARK,  C_NONE);
	gtext(x - 13, y + 25, "Black", C_BLACK, C_NONE);

	x = 76, y = 33;
	grect(x,      y, x + 12, y + 24, C_WHITE);
	grect(x + 13, y, x + 25, y + 24, C_LIGHT);
	grect(x + 26, y, x + 38, y + 24, C_DARK);
	grect(x + 39, y, x + 51, y + 24, C_BLACK);
	gtext(x + 8, y + 1,  "Lighten", C_LIGHTEN, C_NONE);
	gtext(x + 8, y + 9,  "Darken",  C_DARKEN,  C_NONE);
	gtext(x + 8, y + 17, "Invert",  C_INVERT,  C_NONE);

	extern bopti_image_t img_profile_mono;
	extern bopti_image_t img_profile_mono_alpha;
	extern bopti_image_t img_profile_gray;
	extern bopti_image_t img_profile_gray_alpha;

	x = 8, y = 32;
	for(int c = 0; c < 8; c++)
	{
		int z = x + 8 * c + 3;
		gline(z, y, z + 3, y + 3, c);
		gline(z - 1, y + 1, z - 3, y + 3, c);
		gline(z + 2, y + 4, z, y + 6, c);
		gline(z - 2, y + 4, z - 1, y + 5, c);
	}

	x = 2, y = 42;
	for(int j = 0; j < 20; j++)
	for(int i = 0; i < 74; i++)
		gpixel(x + i, y + j, (i ^ j) & 1 ? C_BLACK : C_WHITE);
	gimage(x +  2, y + 2, &img_profile_mono);
	gimage(x + 20, y + 2, &img_profile_mono_alpha);
	gimage(x + 38, y + 2, &img_profile_gray);
	gimage(x + 56, y + 2, &img_profile_gray_alpha);

	gupdate();
	getkey();

	gray_stop();
}

#endif /* FX9860G */
