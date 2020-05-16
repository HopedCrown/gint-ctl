#include <gintctl/libs.h>
#include <gint/display.h>
#include <gint/keyboard.h>
#include <libimg.h>

#ifdef FX9860G
#include <gint/gray.h>
static void render(void)
{
	extern img_t img_libimg_swords;

	img_render_vram_gray(img_libimg_swords, 8, 8);
}

void gintctl_libs_libimg(void)
{
	int key = 0;
	gray_start();

	while(key != KEY_EXIT)
	{
		gclear(C_WHITE);
		render();
		gupdate();

		key = getkey().key;
	}

	gray_stop();
}
#endif

#ifdef FXCG50
static void render(void)
{
	extern img_t img_libimg_sq_even;
	extern img_t img_libimg_sq_odd;
	extern img_t img_libimg_even_odd;
	extern img_t img_libimg_odd_even;
	extern img_t img_libimg_train;

	#define sq_even  img_libimg_sq_even
	#define sq_odd   img_libimg_sq_odd
	#define even_odd img_libimg_even_odd
	#define odd_even img_libimg_odd_even
	#define train    img_libimg_train

	img_t in[4] = { sq_even, sq_odd, odd_even, even_odd };
	img_t out = img_vram();

	img_fill(out, 0xffff);
	img_fill(img_sub(out, 138, 0, 28, -1), 0xc618);

	for(int i=0, y=8; i<4; i++, y+=52)
	{
		img_rotate (in[i], img_at(out,   8, y),     90);
		img_rotate (in[i], img_at(out,   8, y+26),   0);
		img_rotate (in[i], img_at(out,  34, y),    180);
		img_rotate (in[i], img_at(out,  34, y+26), 270);
		img_upscale(in[i], img_at(out,  60, y+1),  2);
		img_hflip  (in[i], img_at(out, 110, y));
		img_vflip  (in[i], img_at(out, 110, y+26));
		img_render (in[i], img_at(out, 140, y+13));
		img_dye    (in[i], img_at(out, 248, y),    0x25ff);
		img_dye    (in[i], img_at(out, 248, y+26), 0xfd04);

		img_t light = img_copy(in[i]);
		img_t dark  = img_copy(in[i]);

		for(int k=0, x=172; k<=2; k++, x+=26)
		{
			img_whiten(light, light);
			img_darken(dark, dark);

			img_render(light, img_at(out, x, y));
			img_render(dark,  img_at(out, x, y+26));
		}

		img_destroy(light);
		img_destroy(dark);
	}

	img_t light = img_lighten_create(train);
	img_t dark  = img_darken_create(train);

	img_render(light, img_at(out, 282,       24));
	img_render(train, img_at(out, 282,    56+24));
	img_render(dark,  img_at(out, 282, 56+56+24));

	img_destroy(light);
	img_destroy(dark);
}

void gintctl_libs_libimg(void)
{
	int key = 0;

	while(key != KEY_EXIT)
	{
		dclear(C_WHITE);
		render();
		dupdate();

		key = getkey().key;
	}
}
#endif
