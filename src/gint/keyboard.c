#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/drivers/keydev.h>
#include <gint/gint.h>
#include <gint/clock.h>

#include <gintctl/util.h>
#include <gintctl/assets.h>

#if GINT_RENDER_RGB
struct keybgrect {
	/* Background rectangle */
	u8 x, y, w, h;
};
struct keysprite {
	/* Coordinates within final rendered image */
	u8 x, y;
	/* Size of rectangle */
	u8 w, h;
	/* sx, sy: Position of label sprite in spritesheet (height 32 pixels) */
	/* lw, lh: Size of label sprite in spritesheet */
	/* lx, ly: Offset of key to label */
	u8 sx;
	u8 lw;
	u8 sy: 5;
	u8 ly: 3;
	u8 lh: 4;
	u8 lx: 4;
	/* Keycode */
	u8 code;
};
_Static_assert(sizeof(struct keysprite) == 9);

struct kbdmodel {
	image_t *label_sheet;
	i16 bg_count;
	i16 key_count;
	struct keybgrect *bgs;
	struct keysprite *keys;
};

#if GINT_HW_CG
  extern struct kbdmodel const img_kbd_sprite_cg;
  #define KBD_SPRITE img_kbd_sprite_cg
#elif GINT_HW_CP
  extern struct kbdmodel const img_kbd_sprite_cp;
  #define KBD_SPRITE img_kbd_sprite_cp
#endif

static void render_keyboard(keydev_t *d, int x0, int y0)
{
	int BG = C_RGB(26, 26, 26);

	for(int i = 0; i < KBD_SPRITE.bg_count; i++) {
		struct keybgrect *r = &KBD_SPRITE.bgs[i];
		drect(x0+r->x, y0+r->y, x0+r->x + r->w-1, y0+r->y + r->h-1, BG);
	}

	for(int i = 0; i < KBD_SPRITE.key_count; i++) {
		struct keysprite *k = &KBD_SPRITE.keys[i];
		int bg = BG, fg = C_BLACK;

		if(keydev_keydown(d, k->code))
			bg = C_BLACK, fg = C_WHITE;

		drect(x0+k->x, y0+k->y, x0+k->x + k->w-1, y0+k->y + k->h-1, bg);
		dsubimage_p4_dye(x0 + k->x + k->lx, y0 + k->y + k->ly,
		                 KBD_SPRITE.label_sheet,
		                 k->sx, k->sy, k->lw, k->lh,
		                 DIMAGE_NONE, fg);
	}
}
#endif

#if GINT_RENDER_MONO
void position(int row, int col, int *x, int *y, int *w, int *h)
{
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
}

static void render_keyboard(keydev_t *d, int x0, int y0)
{
	int x=0, y=0, w=0, h=0;
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
#endif

static void render_option(int x, int y, char const *name, bool enabled)
{
	int w, h;
	dsize(name, NULL, &w, &h);

	if(enabled) drect(x-_(1,2), y-_(1,2), x+w+_(0,1), y+h+_(0,1), C_BLACK);
	dtext(x, y, (enabled ? C_WHITE : C_BLACK), name);
}

static void render_icon(int x, int y, int id)
{
	int left   = _(6,9) * id + _(1, 0);
	int top    = _(id != 3  && id != 4, 0);
	int width  = _(5, 9);
	int height = _(7 - 2*top, 9);
	dsubimage(x, y+top, &img_kbd_events, left,top,width,height, DIMAGE_NONE);
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
	char const *key_names_0xa[] = {
		"KBD", "x", "y", "z", "=",
		"ON", "|<", ">|", "PGUP", "PGDN", "SETTINGS", "OK", "CATALOG",
		"Sqrt", "Exp",
	};

	if(key == 0) return "N/A";
	if(key == KEY_ACON) key = 0x45;
	int row = 9 - (key >> 4);
	int col = (key & 15) - 1;
	if((key & 0xf0) == 0xa0) return key_names_0xa[col];
	return key_names[6*row + col];
}

static void render(keydev_t *d, key_event_t *last_events, int counter)
{
	key_event_t ev;
	dclear(C_WHITE);

	#if GINT_RENDER_RGB
	row_title("Keyboard state visualizer");
	int y0=190, dy=14, maxev=12;
	int x1=290, x2=350;

	if(DWIDTH <= 320)
		x1 -= 190, x2 -= 190, y0 += 120;
	#endif

	#if GINT_RENDER_MONO
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
		render_icon(x1, y+_(0,2), (t==KEYEV_DOWN ? 0 : (t==KEYEV_UP ? 1 : 2)));
		dtext(x1 + _(7,11), y + 1, C_BLACK, key_name(ev.key));

		if(ev.shift) render_icon(x2, y+_(0,1), 3);
		if(ev.alpha) render_icon(x2 + _(5,9), y+_(0,1), 4);
	}

	render_keyboard(d, _(2,10), _(6,21));
	int tr = d->tr.enabled;

	#if GINT_RENDER_MONO
	dtext(35, 10, C_BLACK, "Shift:");
	render_option(35, 16, "Del", (tr & KEYDEV_TR_DELAYED_SHIFT) != 0);
	render_option(47, 16, "Ins", (tr & KEYDEV_TR_INSTANT_SHIFT) != 0);
	dtext(35, 24, C_BLACK, "Alpha:");
	render_option(35, 30, "Del", (tr & KEYDEV_TR_DELAYED_ALPHA) != 0);
	render_option(47, 30, "Ins", (tr & KEYDEV_TR_INSTANT_ALPHA) != 0);
	render_option(35, 38, "-Mods", (tr & KEYDEV_TR_DELETE_MODIFIERS) != 0);
	render_option(35, 46, "-Rels", (tr & KEYDEV_TR_DELETE_RELEASES) != 0);
	render_option(64, 9, "Reps", (tr & KEYDEV_TR_REPEATS) != 0);

	render_icon(54,  9, d->delayed_shift ? 7 : (d->pressed_shift ? 6 : 5));
	render_icon(55, 23, d->delayed_alpha ? 7 : (d->pressed_alpha ? 6 : 5));

	dtext(65, 17, C_BLACK, key_name(d->rep_key));
	dprint(65, 29, C_BLACK, "L:%d", d->events_lost);
	dfont(old_font);
	#endif /* FX9860G */

	#if GINT_RENDER_RGB
	int x3 = 200, y3 = 30;
	if(DWIDTH <= 320)
		x3 -= 190, y3 += 120;

	dtext(x3, y3, C_BLACK, "Shift:");
	render_icon(x3+45, y3, d->delayed_shift ? 7 : (d->pressed_shift ? 6 : 5));
	render_option(x3+5, y3+12, "Del", (tr & KEYDEV_TR_DELAYED_SHIFT) != 0);
	render_option(x3+40, y3+12, "Ins", (tr & KEYDEV_TR_INSTANT_SHIFT) != 0);

	dtext(x3, y3+34, C_BLACK, "Alpha:");
	render_icon(x3+44, y3+34, d->delayed_alpha ? 7 : (d->pressed_alpha ? 6 : 5));
	render_option(x3+5, y3+46, "Del", (tr & KEYDEV_TR_DELAYED_ALPHA) != 0);
	render_option(x3+40, y3+46, "Ins", (tr & KEYDEV_TR_INSTANT_ALPHA) != 0);

	render_option(x3, y3+64, "DelMods", (tr & KEYDEV_TR_DELETE_MODIFIERS) != 0);
	render_option(x3, y3+78, "DelRels", (tr & KEYDEV_TR_DELETE_RELEASES) != 0);

	render_option(x3, y3+96, "Reps", (tr & KEYDEV_TR_REPEATS) != 0);
	dprint(x3, y3+110, C_BLACK, "%s (%d)", key_name(d->rep_key),
		d->rep_count);

	dprint(x3, y3+128, C_BLACK, "Q:%d/%d", d->queue_next, d->queue_end);
	dprint(x3, y3+144, C_BLACK, "Lost:%d", d->events_lost);
	#endif /* FXCG50 */
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
	if(ev.key == KEY_F5) tr.enabled ^= (KEYDEV_TR_DELETE_MODIFIERS |
	                                    KEYDEV_TR_DELETE_RELEASES);
	if(ev.key == KEY_F6) tr.enabled ^= KEYDEV_TR_REPEATS;

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
		while((ev = keydev_read(d, false, NULL)).type == KEYEV_NONE)
			sleep();

		last_events[counter] = ev;
		counter = (counter+1) % 16;
		if(handle_event(d, last_events, counter)) break;

		while((ev = keydev_read(d, false, NULL)).type != KEYEV_NONE && loop)
		{
			last_events[counter] = ev;
			counter = (counter+1) % 16;
			if(handle_event(d, last_events, counter)) loop = false;
		}
	}

	keydev_set_transform(d, tr);
}
