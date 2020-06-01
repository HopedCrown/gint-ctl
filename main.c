#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#define GINT_NEED_VRAM
#include <gint/timer.h>
#include <gint/clock.h>
#include <gint/rtc.h>
#include <gint/keyboard.h>
#include <gint/display.h>


#ifdef FXCG50
extern void PrintXY(int x, int y, const char *str, int, int);
extern void Bdisp_PutDisp_DD(void);
extern void Bdisp_AllClr_VRAM();

#define color_white 0xffff
#endif

void print_bin(int x, int y, uint32_t bin, int digits)
{
	char str[33];
	str[digits] = 0;

	while(--digits >= 0)
	{
		str[digits] = '0' + (bin & 1);
		bin >>= 1;
	}

	print(x, y, str);
}

#if 0
int callback(void *arg)
{
	volatile int *counter = arg;
	(*counter)++;
	return *counter == 7;
}

void test_clock(void)
{
	const clock_frequency_t *freq = clock_freq();

	dclear(color_white);
	print(1, 1, "FLL: %8d", freq->FLL);
	print(1, 2, "PLL: %8d", freq->PLL);

	print(1, 3, "div1: B%3d I%3d P%3d",
		freq->Bphi_div, freq->Iphi_div, freq->Pphi_dev);

	print(1, 5, "Bphi = %10d", freq->Bphi_f);
	print(1, 6, "Iphi = %10d", freq->Iphi_f);
	print(1, 7, "Pphi = %10d", freq->Pphi_f);

	dupdate();
	getkey();

	volatile unsigned int *FRQCRA	= (void *)0xa4150000;
	volatile unsigned int *FRQCRB	= (void *)0xa4150004;
	volatile unsigned int *PLLCR	= (void *)0xa4150024;
	volatile unsigned int *FLLFRQ	= (void *)0xa4150050;

	dclear(color_white);
	print(1, 1, "%8x", *FRQCRA);
	print(1, 2, "%8x", *FRQCRB);
	print(1, 3, "%8x", *PLLCR);
	print(1, 4, "%8x", *FLLFRQ);
	dupdate();
	getkey();
}

void test_timer_simultaneous(void)
{
	volatile int counters[9] = { 0 };
	int count = timer_count();

	for(int tid = 0; tid < count; tid++)
	{
		timer_setup(tid, timer_delay(tid, 1000000), timer_default,
			callback, (void *)&counters[tid]);
	}

	for(int tid = 0; tid < count; tid++) timer_start(tid);

	int limit;

	#ifdef FX9860G
	limit = 4000;
	#else
	limit = 500;
	#endif

	for(int i = 0; i < limit; i++)
	{
		dclear(color_white);
		for(int k = 0; k < 9; k++)
			print(2 * k + 1, 1, "%1x", counters[k]);

		print(1, 8, "%4d", i);
		dupdate();
	}

	for(int tid = 0; tid < count; tid++) timer_free(tid);
}

void test_rtc_time(void)
{
	rtc_time_t time;
	int limit;

	#ifdef FX9860G
	limit = 1500;
	#else
	limit = 200;
	#endif

	for(int i = 0; i < limit; i++)
	{
		const char *days[7] = {
			"Sunday", "Monday", "Tuesday", "Wednesday",
			"Thursday", "Friday", "Saturday",
		};
		dclear(color_white);
		rtc_get_time(&time);

		print(1, 1, "%2d:%2d:%2d",
			time.hours, time.minutes, time.seconds);
		print(1, 2, "%2d-%2d-%4d %s",
			time.month_day, time.month, time.year,
			days[time.week_day]);

		print(1, 8, "%4d", i);
		dupdate();
	}
}

volatile int test_rtc_int_flag = 0;
int test_rtc_int_callback(UNUSED void *arg)
{
	static int count = 0;
	count++;
	print(7, 1, "%2d", count);
	Bdisp_PutDisp_DD();

	test_rtc_int_flag = (count == 10);
	return test_rtc_int_flag;
}
void test_rtc_int(void)
{
	test_rtc_int_flag = 0;
	rtc_start_timer(rtc_1Hz, test_rtc_int_callback, NULL);

	uint32_t vbr = 0x8800df00;
	vbr += 0x600;
	vbr += 0x20;
	vbr += 0xaa0 - 0x400;
	vbr += 20;

	Bdisp_AllClr_VRAM();
	print(1, 1, "count=");
	Bdisp_PutDisp_DD();
	while(!test_rtc_int_flag) __asm__("sleep");
	delay(100);

	/* Not needed since the callback stops the timer
	rtc_stop_timer(); */
}

/* Count how many interrupts I can process in a second */
volatile int test_timer_stress_counter = 0;
volatile int test_timer_stress_done = 0;
int test_timer_stress_callback(void *arg)
{
	volatile int *counter = arg;
	(*counter)++;
	return 0;
}
int test_timer_stress_stop(UNUSED void *arg)
{
	timer_pause(0);
	timer_free(0);

	test_timer_stress_done = 1;
	return 1;
}
int test_timer_stress_start(UNUSED void *arg)
{
	rtc_start_timer(rtc_1Hz, test_timer_stress_stop, NULL);
	timer_start(0);
	return 0;
}
void test_timer_stress(void)
{
	int length = 0;
	gint_intlevel(isSH3() ? 3 : 40, 15);

	Bdisp_AllClr_VRAM();
	print(1, 1, "Testing...");
	Bdisp_PutDisp_DD();

	timer_setup(0, timer_delay(0, length), timer_default,
		test_timer_stress_callback, (void*)&test_timer_stress_counter);
	rtc_start_timer(rtc_1Hz, test_timer_stress_start, NULL);

	do __asm__("sleep");
	while(!test_timer_stress_done);

	Bdisp_AllClr_VRAM();
	print(1, 1, "Test finished!");
	print(1, 2, "length = %8d ms", length);
	print(1, 3, "Processed interrupts:");
	print(1, 4, "%7d", test_timer_stress_counter);
	Bdisp_PutDisp_DD();

	gint_intlevel(isSH3() ? 3 : 40, 5);
	delay(60);
}
#endif

#define TEST_TIMER_FREQ_LIMIT 64
#define TEST_TIMER_FREQ_RTC   RTC_16Hz
#define TEST_TIMER_FREQ_TIMER 62500
#define TEST_TIMER_FREQ_TID   7

#include <gint/mpu/rtc.h>

int test_timer_freq_callback(volatile void *arg)
{
	volatile int *counter = arg;
	(*counter)++;
	return (*counter == TEST_TIMER_FREQ_LIMIT);
}
int test_timer_freq_start(volatile void *arg)
{
	rtc_start_timer(TEST_TIMER_FREQ_RTC, test_timer_freq_callback, arg);
	timer_start(TEST_TIMER_FREQ_TID);
	return 0;
}
void test_timer_freq(void)
{
	volatile int tmu = 0, rtc = 0;
	int tid = TEST_TIMER_FREQ_TID;

	timer_setup(tid, timer_delay(tid, TEST_TIMER_FREQ_TIMER),
		timer_default, test_timer_freq_callback, &tmu);
	rtc_start_timer(TEST_TIMER_FREQ_RTC, test_timer_freq_start, &rtc);

	while(rtc < TEST_TIMER_FREQ_LIMIT)
	{
		dclear(color_white);
		print(1, 1, "TMU %4x", tmu);
		print(1, 2, "RTC %4x", rtc);

		dupdate();
	}
}

#ifdef FX9860G
void fx_frame1(void)
{
	dclear(color_white);
	for(int x = 0; x < 256; x += 4) vram[x] = vram[x + 1] = 0xffffffff;
	for(int r = 0; r <= 8; r += 2)
	{
		for(int x = r; x < 64 - r; x++)
		{
			dpixel(x, r, color_reverse);
			dpixel(x, 127 - r, color_reverse);
		}
		for(int y = r + 1; y < 128 - r; y++)
		{
			dpixel(r, y, color_reverse);
			dpixel(63 - r, y, color_reverse);
		}
	}
	dupdate();
}
void fx_frame2(void)
{
	dclear(color_white);
	for(int x = 20; x <= 88; x += 68)
	for(int y = 10; y <= 44; y += 34)
	{
//		drect(x, y, 20, 10, color_black);
	}

	dline(64, 32, 84, 32, color_reverse);
	dline(64, 32, 78, 18, color_reverse);
	dline(64, 32, 64, 12, color_reverse);
	dline(64, 32, 50, 18, color_reverse);
	dline(64, 32, 44, 32, color_reverse);
	dline(64, 32, 44, 46, color_reverse);
	dline(64, 32, 64, 52, color_reverse);
	dline(64, 32, 78, 46, color_reverse);
}
#endif

typedef void asm_text_t(uint32_t *v1, uint32_t *v2, uint32_t *op, int height);
extern asm_text_t *topti_asm_text[8];

static void show_bootlog(void)
{
	extern char gint_bootlog[22*9];

	int i = 0;
	for(int y = 0; y < 9; y++)
	{
		for(int x = 0; x < 21; x++)
		{
			if(!gint_bootlog[i]) gint_bootlog[i] = ' ';
			i++;
		}
		gint_bootlog[i] = 0;
		i++;
	}

	dclear(color_white);

	for(int y = 0; y < 9; y++)
		print(1, y + 1, gint_bootlog + 22 * y);
	dupdate();

	getkey();
}

int main(GUNUSED int isappli, GUNUSED int optnum)
{
#ifdef FX9860G

//	extern image_t pattern;
//	extern image_t pattern2;

/*	image_t *img = &pattern;

	uint32_t *data = (void *)&img->data;

	for(int i = 0; i < 32; i++) vram[4 * i] = data[i];
	dupdate();
	getkey();

	fx_frame1();
	getkey();
	fx_frame2();
	getkey(); */

	int x = 0, y = 0, k = 0, w, h;

	while(k != KEY_EXIT)
	{
		dclear(color_white);
//		bopti_render_clip(x, y, &pattern2, 0, 0, 48, 16);
		dtext(x, y, "Hello, World!", color_white, color_black);
		dsize("Hello, World!", NULL, &w, &h);
		drect(x, y, x + w - 1, y + h - 1, color_reverse);
		dupdate();

		k = getkey().key;
		if(k == KEY_LEFT /* && x > 2 */) x-=3;
		if(k == KEY_RIGHT /* && x < 92 */) x+=3;
		if(k == KEY_UP /* && y > 2 */) y-=3;
		if(k == KEY_DOWN /* && y < 30 */) y+=3;
	}

#endif

/* #ifdef FXCG50
	initial_timer_status();
	Bdisp_PutDisp_DD();
	getkey();
#endif */

#ifdef FXCG50
	#define rgb(r, g, b) ((r << 11) | (g << 5) | b)

	dclear(0xf800);
	for(int i = 1; i <= 10; i++)
	{
		drect(i, i, 395 - i, 223 - i, (i & 1) ? 0xffff: 0xf800);
	}

	int cx = 100, cy = 100;
	int r = 30;

	drect(cx-r, cy-r, cx+r, cy+r, 0xffff);
	for(int k = -r; k <= r; k += 10)
	{
		dline(cx, cy, cx+k, cy-r, 0x0000);
		dline(cx, cy, cx+k, cy+r, 0x0000);

		dline(cx, cy, cx-r, cy+k, 0x0000);
		dline(cx, cy, cx+r, cy+k, 0x0000);
	}

	for(int y = 0; y < 20; y++)
	for(int x = 0; x < 20; x++)
	{
		dpixel(180+x, 30+y, ((x^y) & 1)
			? rgb(10, 21, 10)
			: rgb(21, 43, 21));
	}

	dupdate();
	getkey();

	for(int y = 0; y < 224; y++)
	for(int x = 0; x < 396; x++)
	{
		int v = y >> 3;
		int h = x >> 3;
		vram[396 * y + x] = 0x3eb7 ^ ((v << 11) + (h << 5) + v);
	}

	volatile int flag = 0;
	int iterations = 0;
	timer_setup(0, timer_delay(0, 1000 * 1000), 0, timer_timeout, &flag);
	timer_start(0);

	while(!flag) r61524_display(vram, 0, 224), iterations++;

	Bdisp_AllClr_VRAM();
	print(1, 1, "%3d FPS", iterations);
	Bdisp_PutDisp_DD();
	getkey();
#endif

	return 0;
}
