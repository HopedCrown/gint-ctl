#ifdef FX9860G

#include <gint/gray.h>
#include <gint/keyboard.h>
#include <gint/hardware.h>
#include <gintctl/gint.h>

#include <string.h>
#include <stdio.h>

/* gintctl_gint_gray(): Gray engine tuning */
void gintctl_gint_gray(void)
{
	uint32_t delays[2];
	dgray_getdelays(&delays[0], &delays[1]);

	int g35pe2 = (gint[HWCALC] == HWCALC_G35PE2);

	int key = 0, sel = 0;
	char str[20];

	dgray(DGRAY_ON);

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);

		dtext(1, 0, C_BLACK, "Gray engine tuning");
		dtext(1, 8, C_BLACK, g35pe2?"Graph 35+E II":"fx-9860G-like");

		sprintf(str, "Light%5u", delays[0]);
		dtext(13, 24, C_BLACK, str);
		sprintf(str, "Dark %5u", delays[1]);
		dtext(13, 32, C_BLACK, str);

		int y = 24 + (sel << 3);
		dtext(7, y, C_BLACK, "<");
		dtext(73, y, C_BLACK, ">");

		drect(96, 16, 127, 31, C_LIGHT);
		drect(96, 32, 127, 47, C_DARK);

		extern bopti_image_t img_opt_gint_gray;
		dimage(0, 56, &img_opt_gint_gray);

		dupdate();
		key = getkey().key;

		if(key == KEY_UP && sel) sel = 0;
		if(key == KEY_DOWN && !sel) sel = 1;

		if(key == KEY_LEFT)
			delays[sel]--;
		else if(key == KEY_RIGHT)
			delays[sel]++;
		else if(g35pe2 && key == KEY_F1)
			delays[0] = 762, delays[1] = 1311;
		else if(g35pe2 && key == KEY_F2)
			delays[0] = 680, delays[1] = 1078;
		else if(g35pe2 && key == KEY_F3)
			delays[0] = 869, delays[1] = 1097;
		else if(g35pe2 && key == KEY_F4)
			delays[0] = 869, delays[1] = 1311;
		else if(g35pe2 && key == KEY_F5)
			delays[0] = 937, delays[1] = 1425;
		else if(!g35pe2 && key == KEY_F1)
			delays[0] = 1075, delays[1] = 1444;
		else if(!g35pe2 && key == KEY_F2)
			delays[0] = 898, delays[1] = 1350;
		else if(!g35pe2 && key == KEY_F3)
			delays[0] = 609, delays[1] = 884;
		else if(!g35pe2 && key == KEY_F4)
			delays[0] = 937, delays[1] = 1333;
		else if(!g35pe2 && key == KEY_F5)
			delays[0] = 923, delays[1] = 1742;
		else continue;

		if(delays[sel] < 100) delays[sel] = 100;
		if(delays[sel] > 3000) delays[sel] = 3000;
		dgray_setdelays(delays[0], delays[1]);
	}

	dgray(DGRAY_OFF);
}

/* gintctl_gint_grayrender(): Gray rendering functions */
void gintctl_gint_grayrender(void)
{
	int x, y;
	int key = 0;

	dgray(DGRAY_ON);
	dclear(C_WHITE);

	dtext(1, 1, C_BLACK, "Gray rendering");

	x = 6, y = 12;
	drect(x,      y,      x + 15, y + 15, C_WHITE);
	drect(x + 16, y,      x + 31, y + 15, C_LIGHT);
	drect(x + 32, y,      x + 47, y + 15, C_DARK);
	drect(x + 48, y,      x + 63, y + 15, C_BLACK);
	drect(x,      y,      x + 63, y +  3, C_LIGHTEN);
	drect(x,      y + 12, x + 63, y + 15, C_DARKEN);

	x = 104, y = 0;
	drect(x, y, x + 23, y + 32, C_BLACK);
	dtext(x - 13, y + 1,  C_WHITE, "White");
	dtext(x - 13, y + 9,  C_LIGHT, "Light");
	dtext(x - 13, y + 17, C_DARK,  "Dark");
	dtext(x - 13, y + 25, C_BLACK, "Black");

	x = 76, y = 33;
	drect(x,      y, x + 12, y + 24, C_WHITE);
	drect(x + 13, y, x + 25, y + 24, C_LIGHT);
	drect(x + 26, y, x + 38, y + 24, C_DARK);
	drect(x + 39, y, x + 51, y + 24, C_BLACK);
	dtext(x + 8, y + 1,  C_LIGHTEN, "Lighten");
	dtext(x + 8, y + 9,  C_DARKEN,  "Darken");
	dtext(x + 8, y + 17, C_INVERT,  "Invert");

	extern bopti_image_t img_profile_mono;
	extern bopti_image_t img_profile_mono_alpha;
	extern bopti_image_t img_profile_gray;
	extern bopti_image_t img_profile_gray_alpha;

	x = 8, y = 32;
	for(int c = 0; c < 8; c++)
	{
		int z = x + 8 * c + 3;
		dline(z, y, z + 3, y + 3, c);
		dline(z - 1, y + 1, z - 3, y + 3, c);
		dline(z + 2, y + 4, z, y + 6, c);
		dline(z - 2, y + 4, z - 1, y + 5, c);

		dline(75, y + c, 83, y + c, c);
		dline(75 + c, y - 12, 75 + c, y - 5, c);
	}

	x = 2, y = 42;
	for(int j = 0; j < 20; j++)
	for(int i = 0; i < 74; i++)
		dpixel(x + i, y + j, (i ^ j) & 1 ? C_BLACK : C_WHITE);
	dimage(x +  2, y + 2, &img_profile_mono);
	dimage(x + 20, y + 2, &img_profile_mono_alpha);
	dimage(x + 38, y + 2, &img_profile_gray);
	dimage(x + 56, y + 2, &img_profile_gray_alpha);

	dupdate();

	while(key != KEY_EXIT) {
		key = getkey().key;
	}

	dgray(DGRAY_OFF);
}

#endif /* FX9860G */
