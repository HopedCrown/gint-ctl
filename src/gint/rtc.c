#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/rtc.h>
#include <gint/timer.h>
#include <gint/clock.h>
#include <gint/gint.h>
#include <gint/std/stdio.h>

#include <gintctl/gint.h>
#include <gintctl/util.h>

#include <libprof.h>

//---
//	Date and time display
//---

extern bopti_image_t img_rtc_segments;

#ifdef FX9860G
char const *days[7] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
char const *months[12] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct",
	"Nov", "Dec"
};

/* Starting x,y and width of each segment image */
static int x0=20, y0=8, dx=13;
/* Starting x and y of date, and alternative y when editing */
static int yd=36, eyd=12;
#endif

#ifdef FXCG50
char const *days[7] = {
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
	"Saturday"
};
char const *months[12] = {
	"January", "February", "March", "April", "May", "June", "July",
	"August", "September", "October", "November", "December"
};

static int x0=89, y0=58, dx=32;
static int yd=121, eyd=121;
#endif

static void draw_time(rtc_time_t *time, int edit_field)
{
	/* Height of characters */
	int sh = img_rtc_segments.height;
	/* Width of colon separator */
	int cw = img_rtc_segments.width - 10 * dx;

	int digits[6] = {
		time->hours / 10,
		time->hours % 10,
		time->minutes / 10,
		time->minutes % 10,
		time->seconds / 10,
		time->seconds % 10,
	};

	for(int i=0, x=x0; i < 6; i++)
	{
		dsubimage(x, y0, &img_rtc_segments, dx*digits[i], 0, dx, sh,
			DIMAGE_NONE);

		x += dx;

		if(i != 1 && i != 3) continue;
		dsubimage(x, y0, &img_rtc_segments, 10*dx,0,cw,sh, DIMAGE_NONE);
		x += cw;
	}

	if(edit_field >= 0)
	{
		int x = x0 + (2 * dx + cw) * edit_field - 1;
		int y = y0 - 1;

		#ifdef FX9860G
		drect(x, y, x + 2*dx - 1, y + sh+1, C_INVERT);

		row_print(6, 1, "EXE:  Set time");
		row_print(7, 1, "EXIT: Cancel");
		#endif

		#ifdef FXCG50
		for(int a = y - 1; a <= y + sh + 2; a++)
		for(int b = x - 1; b <= x + 2*dx - 2; b++)
		{
			gint_vram[396*a+b] ^= 0xffff;
		}

		row_print(12, 1, "EXE:  Set time");
		row_print(13, 1, "EXIT: Cancel");
		#endif

		extern bopti_image_t img_rtc_arrows;
		int xa  = x0 + (2 * dx + cw) * edit_field + dx - 4;
		int ya1 = y - _(1,3) - img_rtc_arrows.height;
		int ya2 = y0 + sh + _(2,4);
		dsubimage(xa, ya1, &img_rtc_arrows, 0,0,7,4, DIMAGE_NONE);
		dsubimage(xa, ya2, &img_rtc_arrows, 8,0,7,4, DIMAGE_NONE);
	}
}

static void draw_date(rtc_time_t *time, int edit_field)
{
	int w[4], h, space_width;
	char month_day[16];
	char year[16];

	sprintf(month_day, "%02d", time->month_day);
	sprintf(year, "%4d", time->year);

	dsize(" ", NULL, &space_width, NULL);
	dsize(days[time->week_day], NULL, &w[0], &h);
	dsize(months[time->month],  NULL, &w[1], &h);
	dsize(month_day,            NULL, &w[2], &h);
	dsize(year,                 NULL, &w[3], &h);

	int y  = (edit_field >= 0) ? eyd: yd;
	int xd = (DWIDTH - (w[0]+w[1]+w[2]+w[3] + 3 * (space_width + 2))) / 2;

	dprint(xd, y, C_BLACK, "%s %s %s %s", days[time->week_day],
		months[time->month], month_day, year);

	if(edit_field >= 0)
	{
		int x = 0;
		for(int i = 0; i < edit_field; i++)
			x += w[i] + space_width + 2;

		#ifdef FX9860G
		drect(xd+x-1, y-1, xd+x+w[edit_field], y+h, C_INVERT);

		row_print(6, 1, "EXE:  Set date");
		row_print(7, 1, "EXIT: Cancel");
		#endif

		#ifdef FXCG50
		for(int a = y-2; a <= y+h; a++)
		for(int b = xd+x-2; b <= xd+x+w[edit_field]+1; b++)
		{
			gint_vram[396*a+b] ^= 0xffff;
		}

		row_print(12, 1, "EXE:  Set date");
		row_print(13, 1, "EXIT: Cancel");
		#endif

		extern bopti_image_t img_rtc_arrows;
		int xa  = xd + x - 3 + (w[edit_field] >> 1);
		int ya1 = y - _(2,4) - img_rtc_arrows.height;
		int ya2 = y + h + _(2,3);
		dsubimage(xa, ya1, &img_rtc_arrows, 0,0,7,4, DIMAGE_NONE);
		dsubimage(xa, ya2, &img_rtc_arrows, 8,0,7,4, DIMAGE_NONE);
	}
}

static void draw_rtc(rtc_time_t *time)
{
	dclear(C_WHITE);
	draw_time(time, -1);
	draw_date(time, -1);

	#ifdef FX9860G
	extern bopti_image_t img_opt_gint_rtc;
	dsubimage(0, 56, &img_opt_gint_rtc, 0, 0, 128, 8, DIMAGE_NONE);
	#endif

	#ifdef FXCG50
	row_title("Real-Time Clock");
	fkey_menu(1, "RTC");
	fkey_menu(2, "TIMER");
	fkey_action(5, "DATE");
	fkey_action(6, "TIME");
	#endif

	dupdate();
}

//---
//	Speed comparison of RTC and timers
//---

static void draw_speed(rtc_time_t *time, uint32_t elapsed)
{
	dclear(C_WHITE);

	#ifdef FX9860G
	extern bopti_image_t img_opt_gint_rtc;
	row_print(1, 1, "Speed of RTC vs TMU");
	dsubimage(0, 56, &img_opt_gint_rtc, 0, 9, 128, 8, DIMAGE_NONE);

	row_print(3, 1, "RTC time: 1/16 s");
	row_print(4, 1, "TMU time: %d us", elapsed);
	#endif

	#ifdef FXCG50
	row_title("Speed comparison of RTC and timers");
	fkey_menu(1, "RTC");
	fkey_menu(2, "TIMER");

	row_print(1, 1, "Run the RTC for 1/16 second.");
	row_print(2, 1, "Time elapsed seen by libprof: %d us", elapsed);
	#endif

	dprint_opt(DWIDTH-2, DHEIGHT-1, C_BLACK, C_NONE, DTEXT_RIGHT,
		DTEXT_BOTTOM, "%s %d, %02d:%02d", months[time->month],
		time->month_day, time->hours, time->minutes);
	dupdate();
}

static prof_t prof;

static int speed_callback(int volatile *flag)
{
	if(*flag == 0)
	{
		prof_enter(prof);
		*flag = 1;
		return TIMER_CONTINUE;
	}
	else
	{
		prof_leave(prof);
		*flag = 2;
		return TIMER_STOP;
	}
}

static uint32_t test_speed(void)
{
	int volatile flag = 0;
	prof = prof_make();

	rtc_start_timer(RTC_16Hz, speed_callback, &flag);

	while(flag != 2) sleep();
	return prof_time(prof);
}

//---
//	Edition facilities
//---

static int days_in_month(rtc_time_t *time)
{
	int base[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	if(time->month != 1) return base[time->month];

	int leap_year = 0;
	if(time->year %   4 == 0) leap_year = 1;
	if(time->year % 100 == 0) leap_year = 0;
	if(time->year % 400 == 0) leap_year = 1;
	return base[time->month] + leap_year;
}

static void normalize(rtc_time_t *time)
{
	/* Check that month_day is still valid */
	int max = days_in_month(time);
	if(time->month_day > max) time->month_day = max;
}

static void move_field(rtc_time_t *time, int field, int diff)
{
	if(field == 0) time->hours = (time->hours + 24 + diff) % 24;
	if(field == 1) time->minutes = (time->minutes + 60 + diff) % 60;
	if(field == 2) time->seconds = (time->seconds + 60 + diff) % 60;
	if(field == 3) time->week_day = (time->week_day + 7 + diff) % 7;

	if(field == 4)
	{
		time->month = (time->month + 12 + diff) % 12;
		normalize(time);
	}
	if(field == 5)
	{
		int max = days_in_month(time);
		time->month_day = ((time->month_day-1) + max + diff) % max + 1;
	}
	if(field == 6)
	{
		time->year += diff;
		if(diff < 0 && -diff > time->year) time->year = 0;
		if(time->year > 9999) time->year = 9999;
		normalize(time);
	}
}

static void set_field(rtc_time_t *time, int field, int value)
{
	if(field == 0) time->hours = value;
	if(field == 1) time->minutes = value;
	if(field == 2) time->seconds = value;
	if(field == 3) time->week_day = value;
	if(field == 4) time->month = value;
	if(field == 5) time->month_day = value;
	if(field == 6) time->year = value;
}

static int valid_field(rtc_time_t *time, int field, int value)
{
	if(field == 0)
		return value >= 0 && value < 24;
	if(field == 1 || field == 2)
		return value >= 0 && value < 60;
	if(field == 3)
		return value >= 0 && value < 7;
	if(field == 4)
		return value >= 0 && value < 12;
	if(field == 5)
		return value >= 1 && value <= days_in_month(time);
	if(field == 6)
		return value >= 0 && value <= 9999;
	return 1;
}

static void edit_time(void)
{
	int key=0, edit_field=0;
	rtc_time_t time;
	rtc_get_time(&time);

	int key_input = 0;
	int key_sequence = 0;

	while(key != KEY_EXE && key != KEY_EXIT)
	{
		dclear(C_WHITE);
		draw_time(&time, edit_field);
		#ifdef FXCG50
		draw_date(&time, -1);
		row_title("Real-Time Clock");
		#endif
		dupdate();

		key = getkey().key;

		if(keycode_digit(key) >= 0)
		{
			/* Try to append the digit to the current value, if it
			   gives an invalid value restart the input sequence */
			key_input = key_input * 10 + keycode_digit(key);
			key_sequence++;

			if(!valid_field(&time, edit_field, key_input))
			{
				key_input = keycode_digit(key);
				key_sequence = 1;
			}

			set_field(&time, edit_field, key_input);

			/* If a field is fully set, switch to the next one */
			if(key_sequence == 2)
			{
				if(edit_field == 2)
				{
					key = KEY_EXE;
					break;
				}
				else
				{
					edit_field++;
					key_input = 0;
					key_sequence = 0;
				}
			}

			continue;
		}

		/* Any key other than a digit breaks an input sequence */
		key_input = 0;
		key_sequence = 0;

		if(key == KEY_LEFT  && edit_field > 0) edit_field--;
		if(key == KEY_RIGHT && edit_field < 2) edit_field++;

		if(key == KEY_UP)   move_field(&time, edit_field, +1);
		if(key == KEY_DOWN) move_field(&time, edit_field, -1);
	}

	if(key == KEY_EXE) rtc_set_time(&time);
}

static void edit_date(void)
{
	int key=0, edit_field=0, option_tab=0;
	rtc_time_t time;
	rtc_get_time(&time);

	int key_input = 0;
	int key_sequence = 0;

	while(key != KEY_EXE && key != KEY_EXIT)
	{
		dclear(C_WHITE);
		#ifdef FXCG50
		draw_time(&time, -1);
		row_title("Real-Time Clock");
		#endif
		draw_date(&time, edit_field);

		#ifdef FX9860G
		extern bopti_image_t img_opt_gint_rtc;
		if(edit_field == 0)
			dsubimage(0, 56, &img_opt_gint_rtc, 0, option_tab*9+18,
				128, 8, DIMAGE_NONE);
		if(edit_field == 1)
			dsubimage(0, 56, &img_opt_gint_rtc, 0, option_tab*9+36,
				128, 8, DIMAGE_NONE);
		#endif

		#ifdef FXCG50
		char const *fkey_days[] = {
			"SUN", "MON", "TUE", "WED", "THUR", "FRI", "SAT"
		};
		if(edit_field == 0) for(int i = 0; i < 5; i++)
		{
			int id = 5 * option_tab + i;
			if(id < 7) fkey_menu(i+1, fkey_days[id]);
		}

		char const *fkey_months[] = {
			"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG",
			"SEP", "OCT", "NOV", "DEC"
		};
		if(edit_field == 1) for(int i = 0; i < 5; i++)
		{
			int id = 5 * option_tab + i;
			if(id < 12) fkey_menu(i+1, fkey_months[id]);
		}

		if(edit_field < 2) fkey_action(6, ">");
		#endif

		dupdate();
		key = getkey().key;

		if(keycode_digit(key) >= 0 && edit_field >= 2)
		{
			/* Try to append the digit to the current value, if it
			   gives an invalid value restart the input sequence */
			key_input = key_input * 10 + keycode_digit(key);
			key_sequence++;

			if(!valid_field(&time, edit_field+3, key_input))
			{
				key_input = keycode_digit(key);
				key_sequence = 1;
			}

			set_field(&time, edit_field+3, key_input);

			/* If a field is fully set, switch to the next one */
			if((edit_field == 2 && key_sequence == 2)
			|| (edit_field == 3 && key_sequence == 4))
			{
				if(edit_field == 3)
				{
					key = KEY_EXE;
					break;
				}
				else
				{
					edit_field++;
					option_tab = 0;
					key_input = 0;
					key_sequence = 0;
				}
			}

			continue;
		}

		/* Any key other than a digit breaks an input sequence */
		key_input = 0;
		key_sequence = 0;

		if(key == KEY_LEFT && edit_field > 0)
		{
			edit_field--;
			option_tab = 0;
		}
		if(key == KEY_RIGHT && edit_field < 3)
		{
			edit_field++;
			option_tab = 0;
		}

		int fk = keycode_function(key);
		if(fk >= 1 && fk <= 5)
		{
			int value = option_tab * 5 + fk - 1;
			if(valid_field(&time, edit_field+3, value))
			{
				set_field(&time, edit_field+3, value);

				/* Go to next field */
				if(edit_field == 3)
				{
					key = KEY_EXE;
					break;
				}
				else
				{
					edit_field++;
					option_tab = 0;
				}
			}
		}
		if(key == KEY_F6)
		{
			int max = (edit_field == 0) ? 1 : 2;
			option_tab++;
			if(option_tab > max) option_tab = 0;
		}

		if(key == KEY_UP)   move_field(&time, edit_field+3, +1);
		if(key == KEY_DOWN) move_field(&time, edit_field+3, -1);
	}

	if(key == KEY_EXE)
	{
		/* Copy current time and set only the date */
		int wd = time.week_day;
		int d = time.month_day;
		int m = time.month;
		int y = time.year;

		rtc_get_time(&time);
		time.week_day = wd;
		time.month_day = d;
		time.month = m;
		time.year = y;
		rtc_set_time(&time);
	}
}

//---
//	Main screen with lazy update
//---

static volatile int frame_done = 0;

static int rtc_timer_callback(void)
{
	frame_done = 0;
	return TIMER_CONTINUE;
}

/* gintctl_gint_rtc(): Configure RTC and check timer speed */
void gintctl_gint_rtc(void)
{
	key_event_t ev;
	rtc_time_t time;

	int run_loop = 1;
	int tab = 1;

	uint32_t elapsed = test_speed();

	rtc_start_timer(RTC_1Hz, rtc_timer_callback);

	while(run_loop)
	{
		if(!frame_done) rtc_get_time(&time);

		/* Redraw only when a second elapses */
		if(!frame_done && tab == 1)
		{
			draw_rtc(&time);
			frame_done = 1;
		}
		else if(!frame_done && tab == 2)
		{
			draw_speed(&time, elapsed);
			frame_done = 1;
		}

		/* Handle keyboard events */
		while((ev = pollevent()).type != KEYEV_NONE)
		{
			if(ev.type != KEYEV_DOWN) continue;
			int action = 1;

			if(ev.key == KEY_EXIT) run_loop = 0;
			else if(ev.key == KEY_MENU) gint_osmenu();
			else if(ev.key == KEY_F1) tab = 1;
			else if(ev.key == KEY_F2) tab = 2;
			else if(ev.key == KEY_F5 && tab == 1) edit_date();
			else if(ev.key == KEY_F6 && tab == 1) edit_time();
			else action = 0;

			if(action) frame_done = 0;
		}

		/* Wait for either keyboard or RTC to produce an event, except
		   if we already have work to do */
		if(frame_done) sleep();
	}

	rtc_stop_timer();
}
