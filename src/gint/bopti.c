#define GINT_NEED_VRAM
#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

#ifdef FXCG50
/* gintctl_gint_bopti(): Test image rendering */
void gintctl_gint_bopti(void)
{
	extern bopti_image_t img_swift;
	extern bopti_image_t img_swords;
	extern bopti_image_t img_potion_17x22, img_potion_18x22;
	extern bopti_image_t img_applejack_31x27, img_applejack_36x25;
	int key = 0, x, y;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);
		row_title("Image rendering");

		x=51, y=25;
		dtext(x,     y, C_BLACK, "E.E");
		dtext(x+35,  y, C_BLACK, "E.O");
		dtext(x+70,  y, C_BLACK, "O.E");
		dtext(x+105, y, C_BLACK, "O.O");

		x=52, y=40;
		dtext(10, y+6, C_BLACK, "OxE");
		dimage(x,     y,   &img_potion_17x22);
		dimage(x+35,  y+1, &img_potion_17x22);
		dimage(x+71,  y,   &img_potion_17x22);
		dimage(x+106, y+1, &img_potion_17x22);

		x=52, y=67;
		dtext(10, y+6, C_BLACK, "ExE");
		dimage(x,     y,   &img_potion_18x22);
		dimage(x+35,  y+1, &img_potion_18x22);
		dimage(x+71,  y,   &img_potion_18x22);
		dimage(x+106, y+1, &img_potion_18x22);

		x=44, y=95;
		dtext(10, y+9, C_BLACK, "OxO");
		dimage(x,     y,   &img_applejack_31x27);
		dimage(x+35,  y+1, &img_applejack_31x27);
		dimage(x+71,  y,   &img_applejack_31x27);
		dimage(x+106, y+1, &img_applejack_31x27);

		x=40, y=127;
		dtext(10, y+9, C_BLACK, "ExO");
		dimage(x,     y,   &img_applejack_36x25);
		dimage(x+35,  y+1, &img_applejack_36x25);
		dimage(x+71,  y,   &img_applejack_36x25);
		dimage(x+106, y+1, &img_applejack_36x25);

		dimage(190, 35, &img_swords);

		dtext(67, 210, C_BLACK,
			"Some images by Pix3M (deviantart.com/pix3m)");
		dupdate();

		key = getkey().key;
	}
}
#endif

#ifdef FX9860G
static void img(int x, int y, bopti_image_t *img, int sub, int flags)
{
	int ix = 0;
	int w = img->width;

	if(sub) x += 2, ix += 2, w -= 4;
	dsubimage(x, y, img, ix, 0, w, img->height, flags);
}
#define img(x, y, i) img(x, y, & img_bopti_##i, sub, flags)

void gintctl_gint_bopti(void)
{
	extern bopti_image_t img_opt_gint_bopti;
	extern bopti_image_t img_bopti_1col;
	extern bopti_image_t img_bopti_2col;
	extern bopti_image_t img_bopti_3col;

	int key = 0;
	int cols=0, flags=DIMAGE_NONE, sub=0, back=0;

	while(key != KEY_EXIT)
	{
		dclear(back ? C_BLACK : C_WHITE);

		if(cols == 0)
		{
			img( -6, 4, 1col);
			img( 20, 4, 1col);
			img( 42, 4, 1col);
			img( 64, 4, 1col);
			img( 90, 4, 1col);
			img(122, 4, 1col);

			dsubimage(0, 10, &img_bopti_1col, 6, 0, 6, 4, flags);
			dsubimage(5, 16, &img_bopti_1col, 6, 0, 6, 4, flags);
			dsubimage(4, 22, &img_bopti_1col, 5, 0, 7, 4, flags);
			dsubimage(3, 28, &img_bopti_1col, 4, 0, 8, 4, flags);
			dsubimage(2, 34, &img_bopti_1col, 3, 0, 9, 4, flags);
			dsubimage(1, 40, &img_bopti_1col, 2, 0,10, 4, flags);
			dsubimage(0, 40, &img_bopti_1col, 1, 0,11, 4, flags);
			dsubimage(0, 46, &img_bopti_1col, 0, 0,12, 4, flags);

			img(33, 12, 1col);
		}
		else if(cols == 1)
		{
			img(-10,  4, 2col);
			img( 28,  4, 2col);
			img( 96,  4, 2col);
			img( 46, 12, 2col);
		}
		else if(cols == 2)
		{
			img(-30,  4, 3col);
			img( 90,  4, 3col);
			img( -4, 12, 3col);
			img( 64, 12, 3col);
		}
		else if(cols == 3)
		{
			img(  0, 4, 1col);
			img( 20, 4, 1col);
			img( 42, 4, 1col);
			img( 64, 4, 1col);
			img(116, 4, 1col);
		}

		dsubimage(0, 56, &img_opt_gint_bopti, 0,0,128,8, DIMAGE_NONE);
		dsubimage(0, 56, &img_opt_gint_bopti, 0, 9*cols, 21, 8,
			DIMAGE_NONE);
		if(flags & DIMAGE_NOCLIP)
		{
			dsubimage(86, 56, &img_opt_gint_bopti, 86, 9, 21, 8,
				DIMAGE_NONE);
		}

		dupdate();

		key = getkey().key;
		if(key == KEY_F1) cols = (cols + 1) % 4;
		if(key == KEY_F4) sub = !sub;
		if(key == KEY_F5) flags ^= DIMAGE_NOCLIP;
		if(key == KEY_F6) back = !back;
	}
}
#endif
