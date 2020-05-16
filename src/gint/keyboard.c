#include <gint/display.h>
#include <gint/keyboard.h>

#include <gintctl/util.h>

void position(int row, int col, int *x, int *y, GUNUSED int *w, GUNUSED int *h)
{
	#ifdef FX9860G
	*y = 10 + 6 * row;
	if(row <= 4) *x = 5 + 6 * col;
	if(row >= 5) *x = 6 + 7 * col;
	#endif

	#ifdef FXCG50
	if(row == 0) *y=2, *x=7+29*col, *w=16, *h=16;
	if(row >= 5) *y=108+23*(row-5), *x=2+35*col, *w=30, *h=17;
	if(row >= 1 && row <= 4)
	{
		*y=28+18*(row-1), *x=2+29*col, *w=25, *h=18;
		if(row == 1 && col == 4) *x=127, *y=37, *w=12, *h=12;
		if(row == 1 && col == 5) *x=140, *y=24, *w=12, *h=12;
		if(row == 2 && col == 4) *x=140, *y=50, *w=12, *h=12;
		if(row == 2 && col == 5) *x=153, *y=37, *w=12, *h=12;
	}
	#endif
}

void render_keyboard(void)
{
	GUNUSED int x, y, w, h;

	#ifdef FXCG50
	extern image_t img_kbd_pressed;
	extern image_t img_kbd_released;
	dimage(15, 21, &img_kbd_released);
	#endif

	for(int row = 0; row < 9; row++)
	{
		int major = (9-row);
		int key_count = (major >= 5) ? 6 : 5;

		for(int col = 0; col < key_count; col++)
		{
			int code = (major << 4) | (col + 1);
			if(code == 0x45) code = 0x07;

			#ifdef FXCG50
			if(!keydown(code)) continue;

			position(row, col, &x, &y, &w, &h);
			dsubimage(15+x, 21+y, &img_kbd_pressed, x,y,w,h, 0);
			#endif

			#ifdef FX9860G
			extern image_t img_keypress;
			extern image_t img_keyrelease;
			position(row, col, &x, &y, &w, &h);
			if(keydown(code)) dimage(x, y, &img_keypress);
			else dimage(x, y, &img_keyrelease);
			#endif
		}
	}
}

static void render(key_event_t *last_events, int counter)
{
	char const *key_names[] = {
		"F1",    "F2",   "F3",   "F4",   "F5",    "F6",
		"SHIFT", "OPTN", "VARS", "MENU", "Left",  "Up",
		"ALPHA", "x^2",  "^",    "EXIT", "Down",  "Right",
		"X,O,T", "log",  "ln",   "sin",  "cos",   "tan",
		"frac",  "F<>D", "(",    ")",    ",",     "->",
		"7",     "8",    "9",    "DEL",  "AC/ON", "0x46",
		"4",     "5",    "6",    "*",    "/",     "0x47",
		"1",     "2",    "3",    "+",    "-",     "0x48",
		"0",     ".",    "x10^", "(-)",  "EXE",   "0x49",
	};

	key_event_t ev;
	dclear(C_WHITE);

	#ifdef FXCG50
	row_title("Keyboard state visualizer");
	int y0=190, dy=14, maxev=12;
	int x1=230, x2=248;
	#endif

	#ifdef FX9860G
	row_print(1, 1, "Keyboard state");
	int y0=52, dy=8, maxev=6;
	int x1=55, x2=67;
	#endif

	for(int i=0, y=y0; i < maxev; i++, y-=dy)
	{
		ev = last_events[(counter+15 - i) % 16];
		if(ev.type == KEYEV_NONE) continue;

		int key = (ev.key == KEY_ACON ? 0x45 : ev.key);
		int row = 9 - (key >> 4);
		int col = (key & 15) - 1;
		char const *name = key_names[6*row + col];

		if(ev.type == KEYEV_UP)
			dprint(x2, y, C_BLACK, C_NONE, "Up %s", name);
		if(ev.type == KEYEV_DOWN)
			dprint(x1, y, C_BLACK,C_NONE,"Down %s", name);
	}

	render_keyboard();
}

int handle_event(key_event_t *last_events, int counter)
{
	key_event_t ev = last_events[(counter + 15) % 16];

	if(ev.type != KEYEV_DOWN) return 0;
	if(ev.key == KEY_EXIT) return 1;
	if(ev.key == KEY_MENU) render(last_events, counter), dupdate(), gint_osmenu();

	return 0;
}

/* gintct_gint_keybaord: Real-time keyboard visualization */
void gintctl_gint_keyboard(void)
{
	/* All initialized with type=KEYEV_NONE */
	key_event_t last_events[16] = { 0 };
	key_event_t ev;
	int counter = 0;

	while(1)
	{
		render(last_events, counter);
		dupdate();

		/* Redraw at each event if needed */
		last_events[counter] = waitevent(NULL);
		counter = (counter+1) % 16;
		if(handle_event(last_events, counter)) return;

		while((ev = pollevent()).type != KEYEV_NONE)
		{
			last_events[counter] = ev;
			counter = (counter+1) % 16;
			if(handle_event(last_events, counter)) return;
		}
	}
}
