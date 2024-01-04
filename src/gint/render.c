#define __BSD_VISIBLE 1
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gintctl/util.h>
#include <math.h>

static void hexa_shape(int x, int y, int r, float alpha)
{
	int xs[6], ys[6];
	for(int i = 0; i < 6; i++) {
		xs[i] = x + r * cosf(alpha + 1.57 + i * (2*M_PI/6));
		ys[i] = y + r * sinf(alpha + 1.57 + i * (2*M_PI/6));
	}

	for(int i = 0; i < 6; i++) {
		int j = (i + 1) % 6;
		dline(xs[i], ys[i], xs[j], ys[j], C_BLACK);
	}
	dline(xs[0], ys[0], xs[2], ys[2], C_BLACK);
	dline(xs[2], ys[2], xs[5], ys[5], C_BLACK);
	dline(xs[5], ys[5], xs[3], ys[3], C_BLACK);
	dline(xs[3], ys[3], xs[1], ys[1], C_BLACK);
	dline(xs[1], ys[1], xs[4], ys[4], C_BLACK);
	dline(xs[4], ys[4], xs[0], ys[0], C_BLACK);
}

void gintctl_gint_render(void)
{
	dclear(C_WHITE);
	drect_border(1, 1, DWIDTH-2, DHEIGHT-2, C_NONE, 1, C_BLACK);

#ifdef FXCG50
	drect_border(3, 3, DWIDTH-4, DHEIGHT-4, C_NONE, 1, C_BLACK);
#endif

	hexa_shape(DWIDTH/2, DHEIGHT/2, _(24, 72), 0.0);

	for(int y = _(3,5); y < DHEIGHT - _(3,5); y++)
	for(int x = _(3,5); x < DWIDTH - _(3,5); x++) {
		if(dgetpixel(x-1, y) == C_BLACK
		|| dgetpixel(x+1, y) == C_BLACK
		|| dgetpixel(x, y-1) == C_BLACK
		|| dgetpixel(x, y+1) == C_BLACK)
			continue;

		if((x ^ y) & 1) dpixel(x, y, C_BLACK);
	}

	dupdate();
	if(getkey().key == KEY_EXIT)
		return;

	dclear(C_WHITE);

	int fill1 = _(C_NONE, C_RGB(24, 24, 24));
	int fill2 = _(C_BLACK, C_RGB(24, 24, 24));

	int x1 = _(5,20), x2 = _(40,90), x3 = _(64,120), x4 = _(97,220);
	int y1 = _(2,20), y2 = _(19,60);
	int w1 = _(26,60), h1 = _(15,30);

	dellipse(x1, y1, x1+w1, y1+h1, fill1, C_BLACK);
	dellipse(x1, y2, x1+w1, y2+4, fill1, C_BLACK);
	dellipse(x1, y2+_(6,10), x1+w1, y2+_(6,10), fill1, C_BLACK);
	dellipse(x2, y1, x2+4, y1+h1, fill1, C_BLACK);
	dellipse(x2+10, y1, x2+20, y1+h1, fill1, C_BLACK);
	dellipse(x3, y1, x3+w1, y1+h1, C_NONE, C_BLACK);
	dellipse(x4, y1, x4+w1, y1+h1, fill2, C_NONE);

	int y3 = _(40,135), y4 = _(55,170);
	int r = _(10,20);
	x1 += w1 / 2;

	dcircle(x1, y3, w1/2, fill1, C_BLACK);
	dcircle(x1, y4+2, 2, fill1, C_BLACK);
	dcircle(x1, y4+_(6,10), 0, fill1, C_BLACK);
	dcircle(x2+r, y3, r, fill1, C_BLACK);
	dcircle(x3+w1/2, y3, r, C_NONE, C_BLACK);
	dcircle(x4+w1/2, y3, r, fill2, C_NONE);

	dupdate();
	if(getkey().key == KEY_EXIT)
		return;
}
