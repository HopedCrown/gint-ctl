#define GINT_NEED_VRAM
#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

/* gintctl_gint_bopti(): Test image rendering */
void gintctl_gint_bopti(void)
{
#ifdef FXCG50
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
#endif
}
