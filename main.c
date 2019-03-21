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


/* print() - formatted printing shorthand */
static void print(int x, int y, const char *format, ...)
{
	char str[45];
	va_list args;
	va_start(args, format);

	vsprintf(str + 2, format, args);

	#ifdef FX9860G
	dtext(6 * (x - 1) + 1, 7 * (y - 1), str + 2, color_black, color_white);
	#endif

	#ifdef FXCG50
	PrintXY(x, y, str, 0, 0);
	#endif

	va_end(args);
}

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

void debug(uint32_t event_code)
{
	print(1, 6, "%8x", event_code);
	dupdate();
}

void debug_exc(uint32_t event_code)
{
	uint32_t spc;
	__asm__("stc spc, %0": "=r"(spc));

	print(1, 1, "EXCEPTION==%8x==", event_code);
	print(1, 2, "SPC=%8x=========", spc);

	if(isSH4())
	{
		volatile uint32_t *TEA = (void *)0xff00000c;
		print(1, 3, "TEA=%8x=========", *TEA);
	}
	dupdate();
}

#if 0
volatile int delay_one_ended = 0;
int stop(__attribute__((unused)) void *arg)
{
	delay_one_ended = 1;
	return 1;
}
void delay_one(void)
{
	int delay_us = 50000;
	static int tid = 1;

/*	if(tid == (isSH3() ? 3 : 7)) tid = (tid + 1) % timer_count();

	delay_one_ended = 0;
	timer_setup(tid, timer_delay(tid,delay_us), timer_default, stop, NULL);
	timer_start(tid);

	while(!delay_one_ended) __asm__("sleep");
	timer_free(tid); */

//	tid = (tid  + 1) % timer_count();
}
void delay(int k)
{
	for(int i = k; i > 0; i--)
	{
#ifdef FX9860G
		Bdisp_ClearLineVRAM(127, 0, 127, 63);
		Bdisp_DrawLineVRAM(127, 0, 127, (63 * i) / k);
		Bdisp_PutDisp_DD();
#endif
		delay_one();
	}
}

#include <gint/timer.h>

int callback(void *arg)
{
	volatile int *counter = arg;
	(*counter)++;
	return *counter == 7;
}

void test_clock(void)
{
	const clock_frequency_t *freq = clock_freq();

	Bdisp_AllClr_VRAM();
	print(1, 1, "FLL: %8d", freq->FLL);
	print(1, 2, "PLL: %8d", freq->PLL);

	print(1, 3, "div1: B%3d I%3d P%3d",
		freq->Bphi_div, freq->Iphi_div, freq->Pphi_dev);

	print(1, 5, "Bphi = %10d", freq->Bphi_f);
	print(1, 6, "Iphi = %10d", freq->Iphi_f);
	print(1, 7, "Pphi = %10d", freq->Pphi_f);

	Bdisp_PutDisp_DD();
	delay(100);

	volatile unsigned int *FRQCRA	= (void *)0xa4150000;
	volatile unsigned int *FRQCRB	= (void *)0xa4150004;
	volatile unsigned int *PLLCR	= (void *)0xa4150024;
	volatile unsigned int *FLLFRQ	= (void *)0xa4150050;

	Bdisp_AllClr_VRAM();
	print(1, 1, "%8x", *FRQCRA);
	print(1, 2, "%8x", *FRQCRB);
	print(1, 3, "%8x", *PLLCR);
	print(1, 4, "%8x", *FLLFRQ);
	Bdisp_PutDisp_DD();
	delay(100);
}

void test_timer_simultaneous(void)
{
	volatile int counters[9] = { 0 };
	int count = timer_count();
	Bdisp_AllClr_VRAM();

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
		Bdisp_AllClr_VRAM();
		for(int k = 0; k < 9; k++)
			print(2 * k + 1, 1, "%1x", counters[k]);

		print(1, 8, "%4d", i);
		Bdisp_PutDisp_DD();
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
		Bdisp_AllClr_VRAM();
		rtc_get_time(&time);

		print(1, 1, "%2d:%2d:%2d",
			time.hours, time.minutes, time.seconds);
		print(1, 2, "%2d-%2d-%4d %s",
			time.month_day, time.month, time.year,
			days[time.week_day]);

		print(1, 8, "%4d", i);
		Bdisp_PutDisp_DD();
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

void display_keys(volatile uint8_t *keys)
{
	for(int r = 0; r < 8; r++) print_bin(1, r + 1, keys[r ^ 1], 8);
	for(int r = 0; r < 4; r++) print_bin(10, r + 1, keys[(r + 8) ^ 1], 8);
}

void keysc_userland(void)
{
	volatile uint16_t *KEYSC = (void *)0xa44b0000;
	uint16_t buffer[6];

	for(int counter = 0; counter < 4000; counter++)
	{
		for(int i = 0; i < 6; i++) buffer[i] = KEYSC[i];
		if(buffer[3] & 0x0800) break;
		dclear(color_white);
		display_keys((volatile uint8_t *)buffer);
		dupdate();
	}
}

int keysc_callback(volatile void *arg)
{
	volatile uint16_t *buf = arg;
	volatile uint16_t *KEYSC = (void *)0xa44b0000;

	for(int i = 0; i < 6; i++) buf[i] = KEYSC[i];
	return 0;
}

void keysc_timer(void)
{
	volatile uint16_t buffer[6] = { 0 };
	int tid = 3;

	/* 32 gives 1024 Hz */
	/* 2048 gives 16 Hz */
#define DELAY 256

	timer_setup(tid, DELAY, 0, keysc_callback, &buffer);
	timer_start(tid);

	dclear(color_white);

#ifdef FX9860G
# define LIMIT 4000
#else
# define LIMIT 500
#endif

	for(int counter = 0; counter < LIMIT; counter++)
	{
		display_keys((volatile uint8_t *)buffer);
		print(18, 8, "%4d", counter);
		dupdate();
	}

	timer_stop(tid);
}

void test_kbd(void)
{
	key_event_t ev;
	key_event_t last = { .type = KEYEV_NONE };
	const char *names[4] = { "NONE", "DOWN", "UP  ", "HOLD" };

	extern int time;
	int pressed[12][8];

	for(int i = 0; i < 12; i++) for(int j = 0; j < 8; j++) pressed[i][j]=0;

	dclear(color_white);

	while(1)
	{
		while((ev = pollevent()).type != KEYEV_NONE)
		{
			last = ev;
			if(ev.type == KEYEV_DOWN && ev.key == KEY_EXIT) return;

			int row = ev.key >> 4;
			int col = ev.key & 0xf;

			if(ev.type == KEYEV_DOWN) pressed[row][col] = 1;
			if(ev.type == KEYEV_UP) pressed[row][col] = 0;
		}

		print(1, 4, "[%4d]", time);
		print(1, 5, "%2x %s", last.key, names[last.type]);

		for(int i = 0; i < 8; i++)
		{
			int x = (i < 8) ? 13 : 2;
			int y = (i < 8) ? 8 - i : 12 - i;

			for(int j = 0; j < 8; j++)
//				Bdisp_SetPoint_VRAM(80 + 2 * j, 2 + 2 * i,
//					pressed[i][j]);
				print(x + j, y, pressed[i][j] ? "#" : "-");
		}

		extern volatile uint8_t state[12];
		print(1,1,"%x %x %x %x",state[0],state[1],state[2],state[3]);
		print(1,2,"%x %x %x %x",state[4],state[5],state[6],state[7]);
		print(1,3,"%x %x %x %x",state[8],state[9],state[10],state[11]);
		dupdate();
	}
}

/* tmu_t - a single timer from a standard timer unit */
typedef volatile struct
{
	uint32_t TCOR;			/* Constant register */
	uint32_t TCNT;			/* Counter register, counts down */

	word_union(TCR,
		uint16_t	:7;
		uint16_t UNF	:1;	/* Underflow flag */
		uint16_t	:2;
		uint16_t UNIE	:1;	/* Underflow interrupt enable */
		uint16_t CKEG	:2;	/* Input clock edge */
		uint16_t TPSC	:3;	/* Timer prescaler (input clock) */
	);

} GPACKED(4) tmu_t;

/* tmu_extra_t - extra timers on sh7337, sh7355 and sh7305 */
typedef volatile struct
{
	uint8_t TSTR;			/* Only bit 0 is used */
	pad(3);

	uint32_t TCOR;			/* Constant register */
	uint32_t TCNT;			/* Counter register */

	byte_union(TCR,
		uint8_t		:6;
		uint8_t UNF	:1;	/* Underflow flag */
		uint8_t UNIE	:1;	/* Underflow interrupt enable */
	);

} GPACKED(4) tmu_extra_t;

/* This is the description of the structure on SH4. SH3-based fx9860g models,
   which are already very rare, will adapt the values in init functions */
tmu_t *std[3] = {
	(void *)0xa4490008,
	(void *)0xa4490014,
	(void *)0xa4490020,
};
tmu_extra_t *ext[6] = {
	(void *)0xa44d0030,
	(void *)0xa44d0050,
	(void *)0xa44d0070,
	(void *)0xa44d0090,
	(void *)0xa44d00b0,
	(void *)0xa44d00d0,
};
tmu_t *std3[3] = {
	(void *)0xfffffe94,
	(void *)0xfffffea0,
	(void *)0xfffffeac,
};
tmu_extra_t *ext3[1] = {
	(void *)0xa44c0030,
};

void initial_timer_status(void)
{
	dclear(color_white);
	print(1, 1, "STR");
	print(1, 2, "TCR");
	print(1, 3, "COR");
	print(1, 4, "CNT");

	print(1, 5, "TCR");
	print(1, 6, "COR");
	print(1, 7, "CNT");

	for(int i = 0; i < 3; i++)
	{
		tmu_t *t = (isSH3() ? std3[i] : std[i]);
		print(5 + 5*i, 5, "%4x", t->TCR.word);
		print(5 + 5*i, 6, (t->TCOR == 0xffffffff) ? "ffff" : "????");
		print(5 + 5*i, 7, (t->TCNT == 0xffffffff) ? "ffff" : "????");
	}

	for(int i = 0; i < (isSH3() ? 1 : 6); i++)
	{
		tmu_extra_t *t = (isSH3() ? ext3[i] : ext[i]);
		print(5 + 3*i, 1, "%2x", t->TSTR);
		print(5 + 3*i, 2, "%2x", t->TCR.byte);
		print(5 + 3*i, 3, (t->TCOR == 0xffffffff) ? "ff" : "??");
		print(5 + 3*i, 4, (t->TCNT == 0xffffffff) ? "ff" : "??");
	}

	dupdate();
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

int main(GUNUSED int isappli, GUNUSED int optnum)
{
	getkey();
#ifdef FX9860G

	extern image_t pattern;
	extern image_t pattern2;

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
