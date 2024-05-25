#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/config.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>
#include <stdlib.h>

#if GINT_RENDER_RGB
static void scene_1(void)
{
	dclear(0x5555);

	extern image_t img_libimg_sq_even;
	extern image_t img_libimg_sq_odd;
	extern image_t img_libimg_even_odd;
	extern image_t img_libimg_odd_even;
	extern image_t img_libimg_train;

	image_t const *in[4] = {
		&img_libimg_sq_even, &img_libimg_sq_odd,
		&img_libimg_odd_even, &img_libimg_even_odd
	};
	image_t *train = &img_libimg_train;
	image_t *vram = image_create_vram();

	image_fill(vram, 0xffff);
	image_fill(image_sub(vram, 138, 0, 28, -1), 0xc618);

	/* TODO: Missing test coverage:
	     image_alloc()
	     image_copy_palette()
	     image_clear()
	     image_copy() with P4 formats
	     image_set_pixel() */

	for(int i=0, y=8; i<4; i++, y+=52)
	{
		/* P8 */
		if(i <= 1) {
			image_t *tmp;

	//		img_rotate (in[i], img_at(vram,   8, y),     90);
	//		img_rotate (in[i], img_at(vram,   8, y+26),   0);
	//		img_rotate (in[i], img_at(vram,  34, y),    180);
	//		img_rotate (in[i], img_at(vram,  34, y+26), 270);
	//		img_upscale(in[i], img_at(vram,  60, y+1),  2);

			if(i == 0) {
				tmp = image_hflip_alloc(in[i]);
				image_copy(tmp, image_at(vram, 110, y), false);
				image_free(tmp);

				tmp = image_vflip_alloc(in[i]);
				image_copy(tmp, image_at(vram, 110, y+26), false);
				image_free(tmp);
			}
			else {
				tmp = image_copy_alloc(in[i], in[i]->format);
				image_hflip(tmp, tmp, true);
				image_copy(tmp, image_at(vram, 110, y), false);
				image_free(tmp);

				tmp = image_copy_alloc(in[i], in[i]->format);
				image_vflip(tmp, tmp, true);
				image_copy(tmp, image_at(vram, 110, y+26), false);
				image_free(tmp);
			}

			image_copy   (in[i], image_at(vram, 140, y+13), false);
	/*		img_dye    (in[i], img_at(vram, 248, y),    0x25ff);
			img_dye    (in[i], img_at(vram, 248, y+26), 0xfd04);

			img_t light = img_copy(in[i]);
			img_t dark  = img_copy(in[i]);

			for(int k=0, x=172; k<=2; k++, x+=26)
			{
				img_whiten(light, light);
				img_darken(dark, dark);

				img_render(light, img_at(vram, x, y));
				img_render(dark,  img_at(vram, x, y+26));
			}

			img_destroy(light);
			img_destroy(dark); */
		}
		/* RGB16 */
		else {
	//		img_rotate (in[i], img_at(vram,   8, y),     90);
	//		img_rotate (in[i], img_at(vram,   8, y+26),   0);
	//		img_rotate (in[i], img_at(vram,  34, y),    180);
	//		img_rotate (in[i], img_at(vram,  34, y+26), 270);
	//		img_upscale(in[i], img_at(vram,  60, y+1),  2);
			image_hflip  (in[i], image_at(vram, 110, y), false);
			image_vflip  (in[i], image_at(vram, 110, y+26), false);
			image_copy   (in[i], image_at(vram, 140, y+13), false);
	/*		img_dye    (in[i], img_at(vram, 248, y),    0x25ff);
			img_dye    (in[i], img_at(vram, 248, y+26), 0xfd04);

			img_t light = img_copy(in[i]);
			img_t dark  = img_copy(in[i]);

			for(int k=0, x=172; k<=2; k++, x+=26)
			{
				img_whiten(light, light);
				img_darken(dark, dark);

				img_render(light, img_at(vram, x, y));
				img_render(dark,  img_at(vram, x, y+26));
			}

			img_destroy(light);
			img_destroy(dark); */
		}
	}

//	img_t light = img_lighten_create(img_libimg_train);
//	img_t dark  = img_darken_create(img_libimg_train);

//	img_render(light, img_at(out, 282,       24));
	image_copy(train, image_at(vram, 282, 56+24), false);
//	img_render(dark,  img_at(out, 282, 56+56+24));

//	img_destroy(light);
//	img_destroy(dark);

	image_free(vram);
}

static void print_linear_map(int x, int y, struct image_linear_map *map)
{
	dprint(x, y, C_BLACK, "Input: %dx%d (stride %d)",
		map->src_w, map->src_h, map->src_stride);
	dprint(x, y+14, C_BLACK, "Output: %dx%d (stride %d)",
		map->dst_w, map->dst_h, map->dst_stride);
	dprint(x, y+28, C_BLACK, "uv initial: %f %f",
		(double)map->u / 65536, (double)map->v / 65536);
	dprint(x, y+42, C_BLACK, "uv per x: %f %f",
		(double)map->dx_u / 65536, (double)map->dx_v / 65536);
	dprint(x, y+56, C_BLACK, "uv per y: %f %f",
		(double)map->dy_u / 65536, (double)map->dy_v / 65536);
}

static void scene_2(void)
{
	dclear(C_WHITE);

	extern image_t img_libimg_train;
	image_t *train = &img_libimg_train;
	image_t *vram = image_create_vram();

	struct image_linear_map map;
	int cx1 = train->width * 3 / 4;
	int cy1 = train->height / 4;
	int cx2=cx1, cy2=cy1;
	image_rotate_around_scale(train, 1.4, 1.3*65536, true, &cx2, &cy2, &map);

	image_t *tmp = image_linear_alloc(train, &map);
	image_copy(tmp, image_at(vram, 10, 10), true);

	dprint(4, 116, C_BLACK, "Center: %d %d -> %d %d", cx1, cy1, cx2, cy2);
	print_linear_map(4, 130, &map);
	image_free(vram);
	image_free(tmp);
}

static void scene_3(image_t **img_ptr, u16 *palette)
{
	image_t *img = *img_ptr;
	if(!img) {
		img = image_alloc(DWIDTH, DHEIGHT, IMAGE_P4_RGB565);
		if(!img)
			return;
		image_set_palette(img, palette, 16, false);
		*img_ptr = img;

		/* Generate a cool looking image pattern */
		for(int i = 0; i < 16; i++) {
			int top = rand() & 31;
			int bot = rand() & top;
			int which = rand() % 3;
			if(which == 0)
				palette[i] = C_RGB(top, bot, bot);
			else if(which == 1)
				palette[i] = C_RGB(bot, top, bot);
			else
				palette[i] = C_RGB(bot, bot, top);
		}

		for(int y = 0; y < DHEIGHT; y++) {
			for(int x = 0; x < DWIDTH; x++) {
				int c1 = x ^ y;
				int c2 = (x >> 5) ^ (x >> 1) ^ (x >> 6);
				int c3 = (y >> 1) ^ (y >> 4) ^ (y >> 5);
				int c = c1 ^ c2 ^ c3;
				image_set_pixel(img, x, y, c & 15);
			}
		}
	}

	dimage(0, 0, img);
}

/* gintctl_gint_image(): Test image rendering */
void gintctl_gint_image(void)
{
	int tab=0, key=0;
	image_t *img_pattern = NULL;
	u16 img_pattern_palette[16];

	while(key != KEY_EXIT) {
		if(tab == 0)
			scene_1();
		else if(tab == 1)
			scene_2();
		else if(tab == 2)
			scene_3(&img_pattern, img_pattern_palette);

		fkey_button(1, "SCENE 1");
		fkey_button(2, "SCENE 2");
		fkey_button(3, "SCENE 3");
		dupdate();

		key = getkey().key;
		if(key == KEY_F1) tab = 0;
		if(key == KEY_F2) tab = 1;
		if(key == KEY_F3) tab = 2;
	}

	image_free(img_pattern);
}
#endif

#if GINT_RENDER_MONO
static void img(int x, int y, bopti_image_t *img, int sub, int flags)
{
	int ix = 0;
	int w = img->width;

	if(sub) x += 2, ix += 2, w -= 4;
	dsubimage(x, y, img, ix, 0, w, img->height, flags);
}
#define img(x, y, i) img(x, y, & img_bopti_##i, sub, flags)

void gintctl_gint_image(void)
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
