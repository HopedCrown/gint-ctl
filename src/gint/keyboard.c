#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/drivers/keydev.h>
#include <gint/gint.h>
#include <gint/clock.h>

#include <gintctl/util.h>
#include <gintctl/assets.h>

static void position(int row, int col, int *x, int *y, int *w, int *h)
{
	#ifdef FX9860G
	*x = 1 + (5 + (row>=5)) * col;
	*y = 1 + 4 * row + (row >= 1) + (row >= 3);
	*w = 4;
	*h = 3 + (row == 0 || row >= 5);
	if(row >= 5) *y += 1 + (row - 5);
	if((row == 1 || row == 2) && col >= 4)
	{
		*x += 3 * (row == 2) - 2 * (col == 5);
		*y += 1 + (row == 1) - 3 * (col == 5);
		*w = 3;
	}
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

static void render_keyboard(keydev_t *d, int x0, int y0)
{
	int x, y, w, h;
	extern bopti_image_t img_kbd_pressed;
	extern bopti_image_t img_kbd_released;
	dimage(x0, y0, &img_kbd_released);

	for(int row = 0; row < 9; row++)
	{
		int major = (9-row);
		int key_count = (major >= 5) ? 6 : 5;

		for(int col = 0; col < key_count; col++)
		{
			int code = (major << 4) | (col + 1);
			if(code == 0x45) code = 0x07;
			if(!keydev_keydown(d, code)) continue;

			position(row, col, &x, &y, &w, &h);
			dsubimage(x0+x, y0+y, &img_kbd_pressed, x,y,w,h, 0);
		}
	}
}

static void render_option(int x, int y, char const *name, bool enabled)
{
	int w, h;
	dsize(name, NULL, &w, &h);

	if(enabled) drect(x, y, x+w+1, y+h+1, C_BLACK);
	dtext(x+1, y+1, (enabled ? C_WHITE : C_BLACK), name);
}

static char const *key_name(int key)
{
	char const *key_names[] = {
		"F1",    "F2",   "F3",   "F4",   "F5",    "F6",
		"SHIFT", "OPTN", "VARS", "MENU", "Left",  "Up",
		"ALPHA", "x^2",  "^",    "EXIT", "Down",  "Right",
		"X,O,T", "log",  "ln",   "sin",  "cos",   "tan",
		"frac",  "F<>D", "(",    ")",    ",",     "->",
		"7",     "8",    "9",    "DEL",  "AC.ON", "0x46",
		"4",     "5",    "6",    "*",    "/",     "0x47",
		"1",     "2",    "3",    "+",    "-",     "0x48",
		"0",     ".",    "x10^", "(-)",  "EXE",   "0x49",
	};

	if(key == 0) return "N/A";
	if(key == KEY_ACON) key = 0x45;
	int row = 9 - (key >> 4);
	int col = (key & 15) - 1;
	return key_names[6*row + col];
}

static void render(keydev_t *d, key_event_t *last_events, int counter)
{

	key_event_t ev;
	dclear(C_WHITE);

	#ifdef FXCG50
	row_title("Keyboard state visualizer");
	int y0=190, dy=14, maxev=12;
	int x1=230, x2=248;
	#endif

	#ifdef FX9860G
	row_print(1, 1, "Keyboard state");
	dimage(0, 56, &img_opt_gint_keyboard);
	int y0=47, dy=7, maxev=6;
	int x1=87, x2=117;

	font_t const *old_font = dfont(&font_mini);
	#endif

	for(int i=0, y=y0; i < maxev; i++, y-=dy)
	{
		ev = last_events[(counter+15 - i) % 16];
		if(ev.type == KEYEV_NONE) continue;

		int t = ev.type;
		dsubimage(x1, y, &img_kbd_events,
			(t == KEYEV_DOWN ? 1 : (t == KEYEV_UP ? 7 : 13)),
			0, 5, 7, DIMAGE_NONE);
		dtext(x1 + 7, y + 1, C_BLACK, key_name(ev.key));

		if(ev.shift)
			dsubimage(x2, y, &img_kbd_events, 19, 0, 5, 7, DIMAGE_NONE);
		if(ev.alpha)
			dsubimage(x2+5, y, &img_kbd_events, 25, 0, 5, 7, DIMAGE_NONE);
	}

	render_keyboard(d, _(2,15), _(10,21));

	#ifdef FX9860G
	int tr = d->tr.enabled;

	dtext(35, 10, C_BLACK, "Shift:");
	render_option(35, 16, "Del", (tr & KEYDEV_TR_DELAYED_SHIFT) != 0);
	render_option(47, 16, "Ins", (tr & KEYDEV_TR_INSTANT_SHIFT) != 0);
	dtext(35, 24, C_BLACK, "Alpha:");
	render_option(35, 30, "Del", (tr & KEYDEV_TR_DELAYED_ALPHA) != 0);
	render_option(47, 30, "Ins", (tr & KEYDEV_TR_INSTANT_ALPHA) != 0);
	render_option(35, 38, "-Mods", (tr & KEYDEV_TR_DELETE_MODIFIERS) != 0);
	render_option(35, 46, "-Rels", (tr & KEYDEV_TR_DELETE_RELEASES) != 0);
	render_option(64, 9, "Reps", (tr & KEYDEV_TR_REPEATS) != 0);

	if(d->delayed_shift)
		dsubimage(54, 10, &img_kbd_events, 43, 1, 5, 5, DIMAGE_NONE);
	else if(d->pressed_shift)
		dsubimage(54, 10, &img_kbd_events, 37, 1, 5, 5, DIMAGE_NONE);
	else
		dsubimage(54, 10, &img_kbd_events, 31, 1, 5, 5, DIMAGE_NONE);

	if(d->delayed_alpha)
		dsubimage(55, 24, &img_kbd_events, 43, 1, 5, 5, DIMAGE_NONE);
	else if(d->pressed_alpha)
		dsubimage(55, 24, &img_kbd_events, 37, 1, 5, 5, DIMAGE_NONE);
	else
		dsubimage(55, 24, &img_kbd_events, 31, 1, 5, 5, DIMAGE_NONE);

	dtext(65, 17, C_BLACK, key_name(d->rep_key));
	dprint(65, 29, C_BLACK, "L:%d", d->events_lost);

	dfont(old_font);
	#endif
}

static int handle_event(keydev_t *d, key_event_t *last_events, int counter)
{
	key_event_t ev = last_events[(counter + 15) % 16];

	if(ev.type != KEYEV_DOWN) return 0;
	if(ev.key == KEY_EXIT) return 1;
	if(ev.key == KEY_MENU)
	{
		render(d, last_events, counter);
		dupdate();
		gint_osmenu();
		return 0;
	}

	keydev_transform_t tr = keydev_transform(d);

	if(ev.key == KEY_F1) tr.enabled ^= KEYDEV_TR_DELAYED_SHIFT;
	if(ev.key == KEY_F2) tr.enabled ^= KEYDEV_TR_INSTANT_SHIFT;
	if(ev.key == KEY_F3) tr.enabled ^= KEYDEV_TR_DELAYED_ALPHA;
	if(ev.key == KEY_F4) tr.enabled ^= KEYDEV_TR_INSTANT_ALPHA;
	if(ev.key == KEY_F5) tr.enabled ^= KEYDEV_TR_REPEATS;
	if(ev.key == KEY_F6) tr.enabled ^= KEYDEV_TR_DELETE_MODIFIERS;

	keydev_set_transform(d, tr);
	return 0;
}

static int repeater(GUNUSED int key, GUNUSED int duration, GUNUSED int count)
{
	if(count == 4) return -1;

	/* Wait 250 ms until the next repeat */
	return 250000;
}

/* gintct_gint_keyboard: Real-time keyboard visualization */
void gintctl_gint_keyboard(void)
{
	keydev_t *d = keydev_std();
	keydev_transform_t tr = keydev_transform(d);

	keydev_transform_t base = { .enabled = 0, .repeater = repeater };
	keydev_set_transform(d, base);

	/* All initialized with type=KEYEV_NONE */
	key_event_t last_events[16] = { 0 };
	key_event_t ev;
	int counter = 0;

	bool loop = true;
	while(loop)
	{
		render(d, last_events, counter);
		dupdate();

		/* Redraw at each event if needed */
		while((ev = keydev_read(d)).type == KEYEV_NONE) sleep();

		last_events[counter] = ev;
		counter = (counter+1) % 16;
		if(handle_event(d, last_events, counter)) break;

		while((ev = keydev_read(d)).type != KEYEV_NONE && loop)
		{
			last_events[counter] = ev;
			counter = (counter+1) % 16;
			if(handle_event(d, last_events, counter)) loop = false;
		}
	}

	keydev_set_transform(d, tr);
}
