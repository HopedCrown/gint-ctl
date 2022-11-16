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

	while(getkey().key != KEY_EXIT) {}
}
