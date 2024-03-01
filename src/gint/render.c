#define __BSD_VISIBLE 1
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/defs/util.h>
#include <gintctl/util.h>
#include <math.h>

#ifdef FXCG50
static void poly2(void)
{
	int x[] = { 0, 400, 400, 450, 0, 2, 223, 253, 274, 350, 121, 2 };
	int y[] = { 200, 200, 250, 150, 200, 62, 236, 222, 236, 184, 2, 62 };
	int N = sizeof(x) / sizeof(x[0]);

	int x_min = x[0];
	int y_min = y[0];
	int x_max = 0;
	int y_max = 0;

	for(int k = 1; k < N; k++) {
		x_min = min(x_min, x[k]);
		x_max = max(x_max, x[k]);
		y_min = min(y_min, y[k]);
		y_max = max(y_max, y[k]);
	}

	float fx = max(1, (float)(x_max - x_min)/320);
	float fy = max(1, (float)(y_max - y_min)/222);

	for(int k = 0; k < N; k++) {
		x[k] = /* DWIDTH - w - 4 + */ ceil((x[k]-x_min)/fx);
		y[k] = /* DHEIGHT - h - 4 + */ ceil((y[k]-y_min)/fy);
	}

	dpoly(x, y, N, C_RGB(15, 15, 31), C_NONE);
}
#endif

void gintctl_gint_render(void)
{
	dclear(C_WHITE);

	int c1 = _(C_BLACK, C_RGB(24, 24, 24));
	int c2 = _(C_BLACK, C_BLACK);
	int c3 = _(C_NONE, C_RGB(24, 24, 24));

	int x1 = _(5,20);
	int x2 = _(40,90);
	int x3 = _(120,360);
	int y1 = _(2,20);
	int y2 = _(19,60);
	int y3 = _(30,135);
	int y4 = _(45,170);
	int y5 = _(50,190);

	int xp = _(90,150);
	int yp = _(25,100);
	int rp = _(20,50);

	drect(x1, y1, x2, y2, c1);
	drect_border(x1, y3, x2, y4, c3, 2, c2);

	dvline(x3, c1);
	dvline(x3+2, c1);
	dvline(x3+4, c1);
	dhline(y5, c2);
	dhline(y5+2, c2);
	dhline(y5+4, c2);

	int px[7], py[7];
	for(int i = 0; i < 7; i++)
	{
		float a = 2 * M_PI * i / 7;
		px[i] = xp + rp * cosf(a);
		py[i] = yp + rp * sinf(a);
	}

	for(int i = 0; i < 7; i++)
		dline(px[i], py[i], px[(i+2) % 7], py[(i+2) % 7], C_BLACK);

#ifdef FXCG50
	int xp2 = xp + rp + 20;
	int yp2 = yp - 80;

	int polyX[7] = { 0, 10, 40, 50, 60, 30, 20 };
	int polyY[7] = { 0, 40, 5, 35, 10, 40, 0 };
	for(int i = 0; i < 7; i++)
	{
		polyX[i] = xp2 + 2 * polyX[i];
		polyY[i] = yp2 + 2 * polyY[i];
	}
	dpoly(polyX, polyY, 7, C_RGB(31, 15, 15), C_BLACK);

	poly2();
#endif

	dupdate();
	while(getkey().key != KEY_EXIT) {}
}
